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

void Facade::rebalance(const map<uri, set<Metadata>>& metadata) {
  for(const auto& pair : metadata) {
    const auto& uri = pair.first;
    auto metadata_set = pair.second;

    // handle metadata only
    if(fs::is_directory(metadata_set)) {
      auto metadata_deleted = metadata_mgr->del(uri);
      const auto status = fs_client->meta(uri, Client::Action::ADD, metadata_deleted);
      if(!status.ok()) {
        logger->warn("failed to add metadata for {} - restoring local metadata.", uri);
        metadata_mgr->add(uri, metadata_deleted);
        //TODO abort? error is likely to be persistent...
      }
    } else if(fs::is_regular_file(metadata_set)) {
      // assert the metadata does no contain hashes,
      // otherwise nodes might think that they're just 
      // updating a file that actually does not exist
      metadata_set = fs::clear_hashes(metadata_set);

      const auto local_path = context.data_directory / uri.path();
      logger->info("trying to rebalance file {}", local_path);
      const bool exists = chord::file::exists(local_path) && chord::file::is_regular_file(local_path);
      if(exists) {
        auto metadata = *metadata_set.begin();

        // handle not replicated data (shallow copy)
        if(metadata.replication == Replication::NONE) {
          metadata.node_ref = context.node();
          set<Metadata> metadata_copy = {metadata};
          const auto status = fs_client->meta(uri, Client::Action::ADD, metadata_copy);
          if(status.ok()) {
            // set node_ref to self to tag the metadata as referenced.
            metadata_mgr->add(uri, metadata_copy);
          } else {
            logger->warn("failed to add shallow copy for {}", uri);
          }

        // handle replications (deep copy)
        } else {
          client::options options;
          metadata.replication.index=0;
          options.replication = metadata.replication;
          options.source = context.uuid();
          options.rebalance = true;
          // note that we put the file from the beginning node to refresh all replications
          const auto status = fs_client->put(uri, local_path, options);
          //if(status.ok() && !metadata.replication.has_next()) {
          //  logger->warn("successfully put uri {} to predecessor - deleting local.", uri);
          //  del(uri);
          //}
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
 * CALLBACKS
 */
// called form within the node that joined the ring
void Facade::on_join(const chord::node new_successor) {
  logger->debug("joined chord ring: new_successor {}", new_successor);
}


//called from within the node preceding the joining node
void Facade::on_joined(const chord::node from_node, const chord::node to_node) {
  logger->debug("node joined: replacing {}, with {}", from_node, to_node);
  const map<uri, set<Metadata>> metadata = metadata_mgr->get_all();
  rebalance(metadata);
}

/**
 * called from within leaving node
 */
void Facade::on_leave(const chord::node predecessor, const chord::node successor) {
  using meta_map = map<uri, set<Metadata>>;
  //FIXME beg refactor 'queries'
  const meta_map metadata_uuids = metadata_mgr->get(predecessor.uuid, context.uuid());
  const meta_map managed_shallow_copies = metadata_mgr->get_shallow_copies(context.node());
  const meta_map replicated = metadata_mgr->get_replicated();

  const auto merge_lambda = [](const set<Metadata> a_value, const set<Metadata> b_value) { 
      auto ret = a_value;
      ret.insert(b_value.begin(), b_value.end());
      return ret;
  };

  const meta_map pt1 = utils::merge<uri, set<Metadata>>(metadata_uuids, managed_shallow_copies, merge_lambda);
  const meta_map metadata = utils::merge<uri, set<Metadata>>(pt1, replicated, merge_lambda);
  //FIXME end refactor 'queries'

  for(const auto& pair : metadata) {
    const auto& uri = pair.first;
    const auto& metadata_set = pair.second;

    //cant do this since router already flushed old nodes
    //const auto hash = chord::crypto::sha256(uri);
    //if(successor == chord->successor(hash)) {
    //  logger->trace("omitting {} since new successor is already responsible", uri);
    //  continue;
    //}

    // handle metadata only
    if(fs::is_directory(metadata_set)) {
      auto metadata_deleted = metadata_set;
      //auto metadata_deleted = metadata_mgr->del(uri);
      const auto status = fs_client->meta(successor, uri, Client::Action::ADD, metadata_deleted);
      if(!status.ok()) {
        logger->warn("failed to add metadata for {} for node {} - restoring local metadata. error: {}", uri, successor, utils::to_string(status));
        //TODO: check whether this makes sense in any way...
        ////metadata_mgr->add(uri, metadata_deleted);
      }
    } else if(fs::is_regular_file(metadata_set)) {
      const auto local_path = context.data_directory / uri.path();
      const bool exists = chord::file::exists(local_path) && chord::file::is_regular_file(local_path);

      if(exists) {
        auto metadata = *metadata_set.begin();
        const auto status = fs_client->put(successor, uri, local_path, {metadata.replication});
        if(!status.ok()) {
          logger->warn("failed to put file {} on successor {}. error: {}", uri, successor, utils::to_string(status));
        } else {
          // TODO make sure this makes sense...
          // set node_ref to self to tag the metadata as referenced.
          ////metadata.node_ref = context.node();
          ////metadata_mgr->add(uri, {metadata});
        }
      } else {
        // handle metadata only
        auto metadata_deleted = metadata_set;
        //auto metadata_deleted = metadata_mgr->del(uri);
        const auto status = fs_client->meta(successor, uri, Client::Action::ADD, metadata_deleted);
        if(!status.ok()) {
          logger->warn("failed to add metadata for {} for node {} - restoring local metadata. error: {}", uri, successor, utils::to_string(status));
          //TODO: check whether this makes sense in any way...
          metadata_mgr->add(uri, metadata_deleted);
        }
      }
    }
  }
}

void Facade::on_predecessor_fail(const chord::node predecessor) {
  logger->warn("detected predecessor failed");
  const map<uri, set<Metadata>> metadata = metadata_mgr->get_replicated(1);
  rebalance(metadata);
}

void Facade::on_successor_fail(const chord::node successor) {
  logger->warn("detected successor failed.");
}


}  // namespace fs
}  // namespace chord
