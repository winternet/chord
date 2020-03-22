#pragma once
#include <grpcpp/impl/codegen/status.h>
#include <map>
#include <set>
#include <iosfwd>
#include <memory>

#include "chord.fs.replication.h"
#include "chord.fs.client.h"
#include "chord.fs.service.h"
#include "chord.i.fs.facade.h"
#include "chord.i.fs.metadata.manager.h"
#include "chord.uri.h"

namespace chord { class path; }
namespace chord { class ChordFacade; }
namespace chord { namespace fs { class MetaResponse; } }
namespace chord { struct Context; }
namespace chord { struct node; }
namespace grpc { class Service; }
namespace spdlog { class logger; }

namespace chord {
namespace fs {

class Facade : public IFacade {
  static constexpr auto logger_name = "chord.fs.facade";

 private:
  const Context& context;
  chord::ChordFacade* chord;
  std::unique_ptr<chord::fs::IMetadataManager> metadata_mgr;
  std::unique_ptr<Client> fs_client;
  std::unique_ptr<Service> fs_service;
  std::shared_ptr<spdlog::logger> logger;

 private:
  grpc::Status put_file(const chord::path& source, const chord::uri& target, Replication repl = Replication());
  grpc::Status get_file(const chord::uri& source, const chord::path& target);
  bool is_directory(const chord::uri& target);
  grpc::Status get_and_integrate(const chord::fs::MetaResponse& metadata);
  grpc::Status get_shallow_copies(const chord::node& leaving_node);
  void rebalance(const std::map<chord::uri, std::set<fs::Metadata>>&);
  grpc::Status rebalance_metadata(const uri&);
 public:
  Facade(Context& context, ChordFacade* chord);

  Facade(Context& context, fs::Client* fs_client, fs::Service* fs_service, fs::IMetadataManager*);

  ::grpc::Service* grpc_service();

  grpc::Status put(const chord::path& source, const chord::uri& target, Replication repl = Replication()) override;

  grpc::Status get(const chord::uri& source, const chord::path& target) override;

  grpc::Status dir(const chord::uri& uri, std::iostream& iostream) override;

  grpc::Status del(const chord::uri& uri, const bool recursive=false) override;

  /**
   * callbacks
   */
  void on_join(const chord::node new_successor);
  void on_leave(const chord::node leaving_node, const chord::node new_predecessor);
  void on_predecessor_update(const chord::node old_predecessor, const chord::node new_predecessor);
  void on_predecessor_fail(const chord::node predecessor);
  void on_successor_fail(const chord::node successor);

};

} //namespace fs
} //namespace chord
