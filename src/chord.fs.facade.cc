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

  return fs_client->put(target, source, repl);
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

/**
 * CALLBACKS
 */
//called from within the node succeeding the joining node
void Facade::on_joined(const chord::node old_predecessor, const chord::node new_predecessor) {
  logger->debug("node joined: old_predecessor {}, new predecessor {}", old_predecessor, new_predecessor);
  const map<uri, set<Metadata>> metadata = metadata_mgr->get(old_predecessor.uuid, new_predecessor.uuid);

  // TODO cleanup! split in several private methods
  for(const auto& pair : metadata) {
    const auto& uri = pair.first;
    auto metadata_set = pair.second;

    // handle metadata only
    if(fs::is_directory(metadata_set)) {

      const auto metadata_deleted = metadata_mgr->del(uri);
      const auto status = fs_client->meta(uri, Client::Action::ADD, metadata_set);
      if(!status.ok()) {
        //logger->warn("failed to add metadata for {} for node {} - restoring local metadata.", uri, new_predecessor);
        metadata_mgr->add(uri, metadata_deleted);
        //TODO abort? error is likely to be persistent...
      }
    } else if(fs::is_regular_file(metadata_set)) {

      // assert the metadata does no contain hashes,
      // otherwise nodes might think that they're just 
      // updating a file that actually does not exist
      metadata_set = fs::clear_hashes(metadata_set);

      const auto local_path = context.data_directory / uri.path();
      if(file::exists(local_path) && file::is_regular_file(local_path)) {
        auto metadata = *metadata_set.begin();

        // handle not replicated data (shallow copy)
        if(metadata.replication == Replication::NONE) {
          metadata.node_ref = context.node();
          set<Metadata> metadata_copy = {metadata};
          const auto status = fs_client->meta(new_predecessor, uri, Client::Action::ADD, metadata_copy);
          if(status.ok()) {
            //TODO check if removing all metadata but keeping the actual file
            //     is a good idea...
            metadata_mgr->del(uri, metadata_copy, true);
          } else {
            logger->warn("failed to add shallow copy for {}", uri);
          }

        // handle replications (deep copy)
        } else {
          // note that we put the file from the beginning node to refresh all replications
          fs_client->put(uri, local_path, metadata.replication);
        }
      } else {
        logger->warn("failed to handle on_joined for metadata {} - not found.", uri);
      }
    } else {
      logger->warn("failed to handle on_joined for metadata {}", uri);
    }
  }
}

/**
 * called from within leaving node
 */
void Facade::on_leave(const chord::node predecessor, const chord::node successor) {
  const map<uri, set<Metadata>> metadata = metadata_mgr->get(predecessor.uuid, context.uuid());
  const auto node = chord->successor();
  for(const auto& pair : metadata) {
    const auto& uri = pair.first;
    for(auto meta:pair.second) {

      const auto local_path = context.data_directory / uri.path();
      const bool exists = chord::file::exists(local_path) && chord::file::is_regular_file(local_path);

      set<Metadata> metadata = {meta};
      if(meta.file_type != type::regular) {
        logger->debug("adding metadata");
        const auto status = fs_client->meta(successor, uri, Client::Action::ADD, metadata);
      } else if(meta.file_type == type::regular && !exists && meta.node_ref) {
        logger->warn("trying to replicate shallow copy {} referencing node {}.", uri, *meta.node_ref);
        const auto status = fs_client->meta(successor, uri, Client::Action::ADD, metadata);
        if(!status.ok()) {
          logger->error("failed to add metadata {} from referencing node {}.", uri, *meta.node_ref);
          continue;
        }
      } else {
        fs_client->put(node, uri, local_path, meta.replication);
      }
    }
  }
}

void Facade::on_predecessor_fail(const chord::node predecessor) {
  //metadata_mgr->get(
  logger->warn("\n\n************** PREDECESSOR FAIL!");
  //TODO implement
}

void Facade::on_successor_fail(const chord::node successor) {
  logger->warn("successor {} failed, rebalancing replications", successor.string());

  if(chord == nullptr) {
    logger->warn("failed to handle on successor fail: chord facade unavailable");
    return;
  }

  const map<uri, set<Metadata>> replicable_meta = metadata_mgr->get_replicable();
  const auto node = chord->successor();
  for(const auto& pair : replicable_meta) {
    const auto& uri = pair.first;
    for(auto meta:pair.second) {

      const auto local_path = context.data_directory / uri.path();
      const bool exists = chord::file::exists(local_path) && chord::file::is_regular_file(local_path);

      if(!exists && meta.node_ref) {
        logger->warn("trying to replicate shallow copy {} referencing node {}.", uri, *meta.node_ref);
        const auto status = fs_client->get(uri, node, local_path);
        if(!status.ok()) {
          logger->error("failed to get shallow copy {} from referencing node {}.", uri, *meta.node_ref);
          continue;
        }
        {
          // reset node_ref
          meta.node_ref.reset();
          metadata_mgr->add(uri, {meta});
        }
      }

      auto repl = meta.replication;
      fs_client->put(node, uri, local_path, ++repl);
    }
  }
}


}  // namespace fs
}  // namespace chord
