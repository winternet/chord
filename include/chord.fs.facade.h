#pragma once
#include <grpcpp/impl/codegen/status.h>
#include <map>
#include <set>
#include <iosfwd>
#include <memory>

#include "chord.fs.monitor.h"
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

bool is_successful(const ::grpc::Status& status);

class Facade : public IFacade {
  static constexpr auto logger_name = "chord.fs.facade";

public:
  enum class RebalanceEvent {
    JOIN,
    PREDECESSOR_FAIL,
    PREDECESSOR_UPDATE,
    LEAVE
  };

 protected:
  const Context& context;
  chord::ChordFacade* chord;
  std::unique_ptr<chord::fs::IMetadataManager> metadata_mgr;
  std::unique_ptr<chord::fs::monitor> monitor;
  std::unique_ptr<Client> fs_client;
  std::unique_ptr<Service> fs_service;
  std::shared_ptr<spdlog::logger> logger;

 private:
  grpc::Status put_file_journal(const chord::path& source);
  grpc::Status put_file(const chord::path& source, const chord::uri& target, Replication repl = Replication());
  grpc::Status get_file(const chord::uri& source, const chord::path& target);
  bool is_directory(const chord::uri& target);
  grpc::Status get_and_integrate(const chord::fs::MetaResponse& metadata);
  grpc::Status get_shallow_copies(const chord::node& leaving_node);

  void rebalance(const IMetadataManager::uri_meta_map_desc&, const RebalanceEvent event);
  void initialize(const IMetadataManager::uri_meta_map_desc&);
  grpc::Status rebalance_metadata(const uri&, const bool=false);

  void on_fs_event(const std::vector<chord::fs::monitor::event> events);
  grpc::Status handle_fs_update(const chord::fs::monitor::event&);
  grpc::Status handle_fs_remove(const chord::fs::monitor::event&);
 public:
  Facade(Context& context, ChordFacade* chord);

  Facade(Context& context, fs::Client* fs_client, fs::Service* fs_service, fs::IMetadataManager*, chord::fs::monitor* = nullptr);

  ~Facade();

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
