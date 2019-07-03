#pragma once
#include <memory>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/security/server_credentials.h>

#include "chord.context.h"
#include "chord.router.h"
#include "chord.types.h"
#include "chord.uuid.h"
#include "chord.client.mock.h"
#include "chord.service.mock.h"
#include "chord.facade.h"
#include "chord.fs.metadata.manager.mock.h"
#include "chord.fs.service.h"
#include "chord.fs.client.h"
#include "chord.fs.facade.h"

#include "util/chord.test.helper.h"
#include "util/chord.test.tmp.dir.h"
#include "util/chord.test.tmp.file.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::InsecureServerCredentials;

using namespace chord::test;

namespace chord {

class MockPeer final {
public:
  explicit MockPeer(const endpoint& endpoint, const TmpDir& data_directory)
    : data_directory(data_directory) {
      context = make_context(chord::uuid::random(), data_directory);
      context.bind_addr = endpoint;
      router = new chord::Router(context);
      client = new MockClient();
      service = new MockService();
      chord_facade = std::make_unique<chord::ChordFacade>(context, router, client, service);
      //--- fs
      metadata_mgr = new chord::fs::MockMetadataManager;
      fs_service = new fs::Service(context, chord_facade.get(), metadata_mgr);
      fs_client = new fs::Client(context, chord_facade.get(), metadata_mgr);
      fs_facade = std::make_unique<chord::fs::Facade>(context, fs_client, fs_service, metadata_mgr);

      ServerBuilder builder;
      builder.AddListeningPort(endpoint, InsecureServerCredentials());
      builder.RegisterService(fs_service);
      server = builder.BuildAndStart();
    }

  ~MockPeer() {
    server->Shutdown();
  }

  //--- chord
  chord::Context context;
  chord::Router* router;
  chord::MockClient* client;
  chord::MockService* service;
  //--- fs
  chord::fs::MockMetadataManager* metadata_mgr;
  std::unique_ptr<chord::ChordFacade> chord_facade;


  chord::fs::Service* fs_service;
  chord::fs::Client* fs_client;
  std::unique_ptr<chord::fs::Facade> fs_facade;
  std::unique_ptr<Server> server;

  // directories
  TmpDir meta_directory;
  const TmpDir& data_directory;
};

} // namespace chord
