#include "chord.fs.facade.h"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <fstream>

#include <grpcpp/impl/codegen/status_code_enum.h>

#include "chord.context.h"
#include "chord.exception.h"
#include "chord.facade.h"
#include "chord.file.h"
#include "chord.fs.metadata.h"
#include "chord.fs.type.h"
#include "chord.fs.metadata.manager.h"
#include "chord.log.factory.h"
#include "chord.log.h"
#include "chord.node.h"
#include "chord.utils.h"

using namespace std;

namespace chord {
namespace fs {

using grpc::Status;
using grpc::StatusCode;

Facade::Facade(Context& context, ChordFacade* chord)
    : context{context},
      chord{chord},
      metadata_mgr{make_unique<chord::fs::MetadataManager>(context)},
      fs_client{make_unique<fs::Client>(context, chord, metadata_mgr.get())},
      fs_service{make_unique<fs::Service>(context, chord, metadata_mgr.get())},
      logger{context.logging.factory().get_or_create(logger_name)}
{}

Facade::Facade(Context& context, fs::Client* fs_client, fs::Service* fs_service, fs::IMetadataManager* metadata_mgr)
  : context{context},
    chord{nullptr},
    metadata_mgr{metadata_mgr},
    fs_client{fs_client},
    fs_service{fs_service},
    logger{context.logging.factory().get_or_create(logger_name)}
{}

::grpc::Service* Facade::grpc_service() {
  return fs_service.get();
}

bool Facade::is_directory(const chord::uri& target) {
  set<Metadata> metadata;
  const auto status = fs_client->dir(target, metadata);
  if (!status.ok()) throw__fs_exception("failed to dir " + to_string(target) + ": "+ status.error_message());
  // check if exists?
  return !metadata.empty();
}

Status Facade::put(const chord::path &source, const chord::uri &target, Replication repl) {
  if (chord::file::is_directory(source)) {
    for(const auto &child : source.recursive_contents()) {
      // dont put empty folders for now
      if(file::is_directory(child)) continue;

      const auto relative_path = child - source.parent_path();
      const auto new_target = target.path().canonical() / relative_path;
      const auto status = put_file(child, {target.scheme(), new_target}, repl);
      //TODO error handling - rollback strategy?
      switch(status.error_code()) {
        case StatusCode::OK: 
          continue;
        case StatusCode::OUT_OF_RANGE:
          logger->warn("Failed to put all replications of{} to {}: {}", source, target, utils::to_string(status));
          continue;
        default:
          return status;
      }
      if(!status.ok()) return status;
    }
  } else {
    const auto new_target = is_directory(target) ? target.path().canonical() / source.filename() : target.path().canonical();
    return put_file(source, {target.scheme(), new_target}, repl);
  }
  return Status::OK;
}

Status Facade::get(const chord::uri &source, const chord::path& target) {

  std::set<Metadata> metadata;
  const auto status = fs_client->meta(source, Client::Action::DIR, metadata);
  if(!status.ok()) return status;

  for(const auto& meta:metadata) {
    // subtract and append, e.g.
    // 1.1) source.path() == /file.txt
    // 2.1) meta.name == /file.txt
    // => /file.txt
    // 1.2) source.path() == /folder/
    // 1.2) meta.name == file.txt
    // => /folder/file.txt
    const auto new_source = (source.path().canonical() - path{meta.name}) / path{meta.name};

    if(meta.file_type == type::regular) {
      // issue get_file
      const auto new_target = file::exists(target) ? target / path{meta.name} : target;
      return get_file({source.scheme(), new_source}, new_target);
    } else if(meta.file_type == type::directory && meta.name != ".") {
      // issue recursive metadata call
      return get({source.scheme(), new_source}, target / path{meta.name});
    }
  }
  return Status::OK;
}

Status Facade::dir(const chord::uri &uri, iostream &iostream) {
  std::set<Metadata> metadata;
  const auto status = fs_client->dir(uri, metadata);
  if(status.ok()) {
    iostream << metadata;
  }
  return status;
}

Status Facade::del(const chord::uri &uri, const bool recursive) {
  return fs_client->del(uri, recursive);
}

Status Facade::put_file(const path& source, const chord::uri& target, Replication repl) {
  // validate input...
  if(repl.count > Replication::MAX_REPL_CNT) {
    return Status(StatusCode::FAILED_PRECONDITION, "replication count above "+to_string(Replication::MAX_REPL_CNT)+" is not allowed");
  }

  return fs_client->put(target, source, {repl});
}

Status Facade::get_file(const chord::uri& source, const chord::path& target) {
  try {
    // assert target path exists
    const auto parent = target.parent_path();
    if(!file::exists(parent)) file::create_directories(parent);

    return fs_client->get(source, target);
  } catch (const std::ios_base::failure& exception) {
    return Status(StatusCode::INTERNAL, "failed to issue get_file ", exception.what());
  }
}

Status Facade::rebalance_metadata(const uri& uri) {
    logger->info("trying to rebalance metadata {}", uri);
    auto metadata_deleted = metadata_mgr->del(uri);
    const auto status = fs_client->meta(uri, Client::Action::ADD, metadata_deleted);
    if(!status.ok()) {
      logger->warn("failed to add metadata for {} - restoring local metadata.", uri);
      metadata_mgr->add(uri, metadata_deleted);
      //TODO abort? error is likely to be persistent...
    }
    return status;
}

void Facade::rebalance(const map<uri, set<Metadata>>& metadata) {
  for(const auto& pair : metadata) {
    const auto& uri = pair.first;
    auto metadata_set = pair.second;

    const auto local_path = context.data_directory / uri.path();
    const bool exists = chord::file::exists(local_path) && chord::file::is_regular_file(local_path);
    const bool is_shallow_copy = !exists && fs::is_shallow_copy(metadata_set);

    // handle metadata only
    if(fs::is_directory(metadata_set) || is_shallow_copy) {
      rebalance_metadata(uri);
    } else {
      logger->info("trying to rebalance file {}", local_path);
      auto metadata = *metadata_set.begin();
      client::options options;
      metadata.replication.index=0;
      options.replication = metadata.replication;
      //options.source = context.uuid();
      options.rebalance = true;
      // note that we put the file from the beginning node to refresh all replications
      const auto status = fs_client->put(uri, local_path, options);
    }
  }
}

/**
 * CALLBACKS
 */
// called form within the node that joined the ring
void Facade::on_join(const chord::node new_successor) {
  logger->debug("[on_join] joined chord ring: new_successor {}", new_successor);
}


//called from within the node preceding the joining node
void Facade::on_joined(const chord::node from_node, const chord::node to_node) {
  logger->debug("[on_joined] node joined: replacing {}, with {}", from_node, to_node);
  const map<uri, set<Metadata>> metadata = metadata_mgr->get_all();
  rebalance(metadata);
}

/**
 * called from the leaving node
 */
void Facade::on_leave(const chord::node predecessor, const chord::node successor) {
  logger->debug("node leaving: informing predecessor {}", predecessor);
  const map<uri, set<Metadata>> metadata = metadata_mgr->get_all();
  rebalance(metadata);
}

void Facade::on_predecessor_fail(const chord::node predecessor) {
  logger->warn("detected predecessor failed");
  //const map<uri, set<Metadata>> metadata = metadata_mgr->get_replicated(1);
  //rebalance(metadata);
}

void Facade::on_successor_fail(const chord::node successor) {
  logger->warn("detected successor failed.");
  const map<uri, set<Metadata>> metadata = metadata_mgr->get_all();
  rebalance(metadata);
}


}  // namespace fs
}  // namespace chord
