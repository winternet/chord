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
  chord::ChordFacade* chord;
  std::unique_ptr<Client> fs_client;
  std::unique_ptr<Service> fs_service;
  std::shared_ptr<spdlog::logger> logger;

 private:
  grpc::Status put_file(const chord::path& source, const chord::uri& target, Replication repl = Replication());
  grpc::Status get_file(const chord::uri& source, const chord::path& target);
  bool is_directory(const chord::uri& target);
  grpc::Status get_and_integrate(const chord::fs::MetaResponse& metadata);
  grpc::Status get_shallow_copies(const chord::node& leaving_node);

 public:
  Facade(Context& context, ChordFacade* chord);

  Facade(Context& context, fs::Client* fs_client, fs::Service* fs_service);

  ::grpc::Service* grpc_service();

  grpc::Status put(const chord::path& source, const chord::uri& target, Replication repl = Replication());

  grpc::Status get(const chord::uri& source, const chord::path& target);

  grpc::Status dir(const chord::uri& uri, std::iostream& iostream);

  grpc::Status del(const chord::uri& uri, const bool recursive=false);

  /**
   * callbacks
   */
  void on_join(const chord::node successor, const chord::node predecessor);
  void on_leave(const chord::node leaving_node, const chord::node new_predecessor);
  void on_predecessor_fail(const chord::node predecessor);
  void on_successor_fail(const chord::node successor);

  chord::take_consumer_t on_leave_callback();

  /**
   * @note should be called only once - create on demand
   */
  chord::take_consumer_t take_consumer_callback();

  /**
   * @note should be called only once - create on demand
   */
  chord::take_producer_t take_producer_callback();
};

} //namespace fs
} //namespace chord
