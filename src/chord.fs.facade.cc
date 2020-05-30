#include "chord.fs.facade.h"

#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <utility>
#include <filesystem>
#include <fstream>

#include <grpcpp/impl/codegen/status_code_enum.h>

#include "chord.fs.monitor.h"
#include "chord.fs.common.h"
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
      monitor{make_unique<fs::monitor>(context)},
      fs_client{make_unique<fs::Client>(context, chord, metadata_mgr.get())},
      fs_service{make_unique<fs::Service>(context, chord, metadata_mgr.get(), monitor.get())},
      logger{context.logging.factory().get_or_create(logger_name)}
{
  monitor->events().connect(this, &Facade::on_fs_event);
}

Facade::Facade(Context& context, fs::Client* fs_client, fs::Service* fs_service, fs::IMetadataManager* metadata_mgr, chord::fs::monitor* monitor)
  : context{context},
    chord{nullptr},
    metadata_mgr{metadata_mgr},
    monitor{monitor},
    fs_client{fs_client},
    fs_service{fs_service},
    logger{context.logging.factory().get_or_create(logger_name)}
{}

::grpc::Service* Facade::grpc_service() {
  return fs_service.get();
}

void Facade::on_fs_event(std::vector<chord::fs::monitor::event> events) {
  events.erase(std::remove_if(events.begin(), events.end(), [&](auto& event) {
      const auto flgs = event.flags;
      const auto removed = std::any_of(flgs.begin(), flgs.end(), [](const auto& f) { return f == monitor::event::flag::REMOVED; });
      const auto not_exists = !metadata_mgr->exists(uri{"chord", path{event.path}-context.data_directory});
      return removed && not_exists;
  }), events.end());
  std::for_each(events.begin(), events.end(), [&](const auto& e) {
      logger->info("received event at {}", e);
  });
  std::for_each(events.begin(), events.end(), [&](const auto& e) {
      const auto flgs = e.flags;

      const auto updated = std::any_of(flgs.begin(), flgs.end(), [](const auto& f) { return f == chord::fs::monitor::event::flag::UPDATED; });
      const auto removed = std::any_of(flgs.begin(), flgs.end(), [](const auto& f) { return f == chord::fs::monitor::event::flag::REMOVED; });

      if(updated) {
        handle_fs_update(e);
      }
      if(removed) {
        handle_fs_remove(e);
      }
  });
}

Status Facade::handle_fs_remove(const chord::fs::monitor::event& event) {
  return Status::OK;
}

Status Facade::handle_fs_update(const chord::fs::monitor::event& event) {
  const auto event_path = path{event.path};
  return put_file_journal(event_path);
}

bool Facade::is_directory(const chord::uri& target) {
  set<Metadata> metadata;
  const auto status = fs_client->dir(target, metadata);
  // XXX here be dragons: what if target is indeed an directory but currently does not exist?
  if(status.error_code() == StatusCode::NOT_FOUND)
    return false;
  else if (!status.ok()) throw__fs_exception("failed to dir " + to_string(target) + ": "+ status.error_message());
  return chord::fs::is_directory(metadata);
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
      //if(!status.ok()) return status;
      if(!is_successful(status)) return status;
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
  //if(repl.count > Replication::MAX_REPL_CNT) {
  //  return Status(StatusCode::FAILED_PRECONDITION, "replication count above "+to_string(Replication::MAX_REPL_CNT)+" is not allowed");
  //}

  const auto data = context.data_directory / target.path();
  return fs_client->put(target, source, client::options{repl});
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

Status Facade::rebalance_metadata(const uri& uri, const bool shallow_copy) {
    logger->info("trying to rebalance metadata {}", uri);
    auto metadata_deleted = metadata_mgr->del(uri);

    if(shallow_copy) {
      std::for_each(metadata_deleted.begin(), metadata_deleted.end(), [&](auto& meta) { 
          if(meta.node_ref) return;
          // set node ref
          if(auto node = metadata_deleted.extract(meta)) {
            node.value().node_ref = context.node();
            metadata_deleted.insert(std::move(node));
          }
      });
    }

    const auto status = fs_client->meta(uri, Client::Action::ADD, metadata_deleted);
    if(!status.ok()) {
      logger->warn("failed to add metadata for {} - restoring local metadata.", uri);
      metadata_mgr->add(uri, metadata_deleted);
      //TODO abort? error is likely to be persistent...
    }
    return status;
}

void Facade::rebalance(const map<uri, set<Metadata>>& metadata, const RebalanceEvent event) {
  for(const auto& pair : metadata) {
    const auto& uri = pair.first;
    auto metadata_set = pair.second;

    const auto local_path = context.data_directory / uri.path();
    const bool exists = chord::file::exists(local_path) && chord::file::is_regular_file(local_path);
    const bool is_shallow_copy = !exists && fs::is_shallow_copy(metadata_set, context);
    const bool is_shallow_copyable = event == RebalanceEvent::PREDECESSOR_UPDATE
                                      && fs::is_shallow_copyable(metadata_set, context);

    // handle metadata only
    if(fs::is_directory(metadata_set) || is_shallow_copy || is_shallow_copyable) {
      rebalance_metadata(uri, is_shallow_copyable);
    } else {
      logger->info("[rebalance] trying to rebalance file {}", local_path);
      auto metadata = *metadata_set.begin();
      client::options options;
      metadata.replication.index=0;
      options.replication = metadata.replication;
      //options.source = context.uuid();
      options.rebalance = true;
      // note that we put the file from the beginning node to refresh all replications
      try {
        const auto status = fs_client->put(uri, local_path, options);
      } catch(chord::exception& e) {
        logger->error("[rebalance] failed to put file {}:{}", uri, e.message());
      }
    }
  }
}

/**
 * CALLBACKS
 */
// called from within the node that joined the ring
void Facade::on_join(const chord::node new_successor) {
  logger->info("[on_join] joined chord ring: new_successor {}", new_successor);
  const map<uri, set<Metadata>> metadata = metadata_mgr->get_all();
  initialize(metadata);
}

/**
 * put a file by moving it to <journal> and put from there
 */
Status Facade::put_file_journal(const path& data_path) {
  auto journal_path = context.journal_directory();

  if(!chord::file::exists(journal_path))
    chord::file::create_directories(journal_path);

  const auto relative_path = data_path - context.data_directory;

  journal_path = journal_path / relative_path;
  const auto lock = monitor::lock(monitor.get(), {data_path});

  // move file to journal_path
  chord::file::rename(data_path, journal_path);

  const auto target = uri{"chord", relative_path};
  const auto status = this->put(journal_path, target, Replication{context.replication_cnt});
  if(status.ok()) {
    logger->info("successfully put after fs_event, removing {}", journal_path);
    chord::file::remove(journal_path);
  }
  //TODO handle failures
  return status;
}

void Facade::initialize(const std::map<chord::uri, std::set<fs::Metadata>>& metadata) {
  namespace fs = std::filesystem;

  logger->info("[initialize] matching data against metadata");
  if(!file::exists(context.data_directory)) return;

  for(const auto& entry : fs::recursive_directory_iterator(context.data_directory.string())) {
    const auto pth = path{entry.path().string()};
    if(!file::is_regular_file(pth)) continue;

    const auto relative_path = pth - context.data_directory;
    const auto uri = chord::uri{"chord", relative_path};
    logger->trace("[initialize] checking: {}", uri);
    if(metadata.find(uri) == metadata.cend()) {
      logger->trace("[initialize] meanwhile created: {}", uri);
      const auto status = put_file_journal(pth);
      if(!status.ok()) {
        logger->error("[initialize] failed to put {} - backup in <metadata>/journal", relative_path);
      }
    }
  }
}

//called from within the node preceding the joining node
void Facade::on_predecessor_update(const chord::node from_node, const chord::node to_node) {
  logger->info("[on_predecessor_update] predecessor updated: replacing {}, with {}", from_node, to_node);
  const map<uri, set<Metadata>> metadata = metadata_mgr->get_all();
  rebalance(metadata, RebalanceEvent::PREDECESSOR_UPDATE);
  if(from_node == context.node() && to_node != context.node()) {
    initialize(metadata);
  }
}

/**
 * called from the leaving node
 */
void Facade::on_leave(const chord::node predecessor, const chord::node successor) {
  logger->debug("[on_leave] node leaving: informing predecessor {}", predecessor);
  if(predecessor == context.node()) {
    logger->debug("[on_leave] node leaving - no node left but self - aborting.");
    return;
  }

  const map<uri, set<Metadata>> metadata = metadata_mgr->get_all();
  rebalance(metadata, RebalanceEvent::LEAVE);
}

void Facade::on_predecessor_fail(const chord::node predecessor) {
  logger->warn("[on_predecessor_fail] detected predecessor failed");
  const map<uri, set<Metadata>> metadata = metadata_mgr->get_replicated(1);
  rebalance(metadata, RebalanceEvent::PREDECESSOR_FAIL);
}

void Facade::on_successor_fail(const chord::node successor) {
  logger->warn("[on_successor_fail] detected successor failed.");
}


}  // namespace fs
}  // namespace chord
