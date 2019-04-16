#pragma once

#include <set>

#include "chord.fs.client.h"
#include "chord.fs.service.h"
#include "chord.i.fs.facade.h"
#include "chord.log.h"
#include "chord.uri.h"

namespace chord {
namespace fs {

class Facade : public IFacade {
  static constexpr auto logger_name = "chord.fs.facade";

 private:
  const Context& context;
  std::unique_ptr<Client> fs_client;
  std::unique_ptr<Service> fs_service;
  std::shared_ptr<spdlog::logger> logger;

 private:
  grpc::Status put_file(const chord::path& source, const chord::uri& target, Replication repl = Replication());
  grpc::Status get_file(const chord::uri& source, const chord::path& target);
  bool is_directory(const chord::uri& target);
  void get_and_integrate(const chord::fs::MetaResponse& metadata);
  void get_shallow_copies(const chord::node& leaving_node);

 public:
  Facade(Context& context, ChordFacade* chord);

  Facade(Context& context, fs::Client* fs_client, fs::Service* fs_service);

  ::grpc::Service* grpc_service();

  grpc::Status put(const chord::path& source, const chord::uri& target, Replication repl = Replication());

  grpc::Status get(const chord::uri& source, const chord::path& target);

  grpc::Status dir(const chord::uri& uri, std::iostream& iostream);

  grpc::Status del(const chord::uri& uri, const bool recursive=false);

  void on_join(const chord::node successor, const chord::node predecessor) {
    fs_client->take(predecessor.uuid, context.uuid(), successor, take_consumer_callback());
  }

  void on_leave(const chord::node leaving_node, const chord::node new_predecessor) {
    fs_client->take(new_predecessor.uuid, leaving_node.uuid, leaving_node, on_leave_callback());
    get_shallow_copies(leaving_node);
  }

  void on_predecessor_fail(const chord::node predecessor) {
    const auto metadata_mgr = fs_service->metadata_manager();
    //metadata_mgr->get(
    logger->warn("\n\n************** PREDECESSOR FAIL!");
    //TODO implement
  }

  void on_successor_fail(const chord::node successor) {
    logger->warn("\n\n************** SUCCESSOR FAIL!");
    //TODO implement
  }

  chord::take_consumer_t on_leave_callback() {
    return [&](const chord::fs::TakeResponse& res) {
      if (!res.has_meta()) return;

      const auto meta = res.meta();

      //--- download the files and integrate the metadata
      try {
        get_and_integrate(meta);
      }catch(...){}
    };
  }

  /**
   * @note should be called only once - create on demand
   * @todo move to cc
   */
  chord::take_consumer_t take_consumer_callback() {
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

  /**
   * @note should be called only once - create on demand
   * @todo move to cc
   */
  chord::take_producer_t take_producer_callback() {
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
};

} //namespace fs
} //namespace chord
