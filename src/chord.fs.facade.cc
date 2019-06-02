#include <fstream>

#include "chord.log.h"
#include "chord.exception.h"
#include "chord.fs.facade.h"
#include "chord.common.h"
#include "chord.fs.metadata.h"

using namespace std;

namespace chord {
namespace fs {

using grpc::Status;
using grpc::StatusCode;

Facade::Facade(Context& context, ChordFacade* chord)
    : context{context},
      chord{chord},
      fs_client{make_unique<fs::Client>(context, chord)},
      fs_service{make_unique<fs::Service>(context, chord)},
      logger{context.logging.factory().get_or_create(logger_name)}
{}

Facade::Facade(Context& context, fs::Client* fs_client, fs::Service* fs_service)
  : context{context},
    chord{nullptr},
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

Status Facade::get_shallow_copies(const chord::node& leaving_node) {
  auto metadata_mgr = fs_service->metadata_manager();

  auto shallow_copies = metadata_mgr->get_shallow_copies(leaving_node);
  // integrate the metadata
  {
    for (const auto& [uri, meta_set] : shallow_copies) {
      std::set<Metadata> deep_copies;
      //copy
      for(auto m:meta_set) {
        if (m.file_type == type::regular && m.name == uri.path().filename()) {
          const auto status = get_file(uri, context.data_directory / uri.path());
          if(!status.ok()) {
            return status;
          }
        }
        m.node_ref.reset();
        deep_copies.insert(m);
      }
      metadata_mgr->add(uri, deep_copies);
    }
  }
  return Status::OK;
}

Status Facade::get_and_integrate(const chord::fs::MetaResponse& meta_res) {
  if (meta_res.uri().empty()) {
    return Status(StatusCode::FAILED_PRECONDITION, "failed to get and integrate meta-response due to missing uri", meta_res.uri());
  }

  const auto uri = chord::uri{meta_res.uri()};
  std::set<Metadata> data_set;

  // integrate the metadata
  {
    for (auto data : MetadataBuilder::from(meta_res)) {
      // unset reference id since the node leaves
      data.node_ref = {};
      data_set.insert(data);
    }
    fs_service->metadata_manager()->add(uri, data_set);
  }

  // get the files
  for (const auto& data : data_set) {
    // uri might be a directory containing data.name as child
    // or uri might point to a file with the metadata containing
    // the file's name, we consider only those leaves
    if (data.file_type == type::regular && data.name == uri.path().filename()) {
      const auto status = get_file(uri, context.data_directory / uri.path());
      if(!status.ok()) {
        return status;
      }
    }
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
 * CALLBACKS (Handler)
 */
chord::take_consumer_t Facade::on_leave_callback() {
  return [&](const chord::fs::TakeResponse& res) {
    if (!res.has_meta()) return;

    const auto meta = res.meta();

    //--- download the files and integrate the metadata
    try {
      get_and_integrate(meta);
    }catch(...){}
  };
}
chord::take_consumer_t Facade::take_consumer_callback() {
  return [&](const chord::fs::TakeResponse& res) {
    if(!res.has_meta()) return;

    const auto meta = res.meta();
    if(meta.uri().empty()) {
      logger->error("Received TakeResponse.MetaResponse without uri.");
      return;
    }

    const std::set<Metadata> metadata = MetadataBuilder::from(meta);
    const auto uri = uri::from(meta.uri());
    fs_service->metadata_manager()->add(uri, metadata);
  };
}
chord::take_producer_t Facade::take_producer_callback() {
  return [&](const auto& from, const auto& to) {
    const auto map = fs_service->metadata_manager()->get(from, to);
    std::vector<chord::fs::TakeResponse> ret;
    for (const auto& m : map) {
      chord::fs::TakeResponse res;
      chord::fs::MetaResponse meta;

      meta.set_uri(m.first);

      auto* node_ref = meta.mutable_node_ref();
      node_ref->set_uuid(context.uuid());
      node_ref->set_endpoint(context.bind_addr);

      MetadataBuilder::addMetadata(m.second, meta);
      res.set_id(m.first);
      res.mutable_meta()->CopyFrom(meta);
      //res.mutable_detail()->PackFrom(meta);
      ret.push_back(res);
    }
    return ret;
  };
}

/**
 * CALLBACKS
 */
//called from within the node succeeding the joining node
void Facade::on_joined(const chord::node old_predecessor, const chord::node new_predecessor) {
  //fs_client->take(predecessor.uuid, context.uuid(), successor, take_consumer_callback());
  logger->debug("node joined: old_predecessor {}, new predecessor {}", old_predecessor, new_predecessor);
  const auto metadata_mgr = fs_service->metadata_manager();
  const map<uri, set<Metadata>> metadata = metadata_mgr->get(old_predecessor.uuid, new_predecessor.uuid);
  for(const auto& pair : metadata) {
    const auto& uri = pair.first;
    for(auto meta:pair.second) {

      const auto local_path = context.data_directory / uri.path();
      const bool exists = chord::file::exists(local_path) && chord::file::is_regular_file(local_path);

      if(meta.file_type == type::regular && !meta.node_ref) {
        meta.node_ref = context.node();
      }

      set<Metadata> metadata = {meta};
      const auto status = fs_client->meta(new_predecessor, uri, Client::Action::ADD, metadata);

      if(status.ok()) {
        metadata_mgr->del(uri, metadata);
      } else {
        logger->warn("failed to add shallow copy for {}", uri);
      }

      // FIXME: do not deep copy -> remove the following line
      if(exists) fs_client->put(new_predecessor, uri, local_path, meta.replication.value_or(Replication()));
    }
  }
}

/**
 * called from within leaving node
 */
void Facade::on_leave(const chord::node predecessor, const chord::node successor) {
  //fs_client->take(new_predecessor.uuid, leaving_node.uuid, leaving_node, on_leave_callback());
  //get_shallow_copies(leaving_node);
  const auto metadata_mgr = fs_service->metadata_manager();
  const map<uri, set<Metadata>> metadata = metadata_mgr->get(predecessor.uuid, context.uuid());
  const auto node = make_node(chord->successor(context.uuid()));
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
        fs_client->put(node, uri, local_path, meta.replication.value_or(Replication()));
      }
    }
  }
}

void Facade::on_predecessor_fail(const chord::node predecessor) {
  const auto metadata_mgr = fs_service->metadata_manager();
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

  const auto metadata_mgr = fs_service->metadata_manager();
  const map<uri, set<Metadata>> replicable_meta = metadata_mgr->get_replicable();
  const auto node = make_node(chord->successor(context.uuid()));
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

      auto repl = *meta.replication;
      fs_client->put(node, uri, local_path, ++repl);
    }
  }
}


}  // namespace fs
}  // namespace chord
