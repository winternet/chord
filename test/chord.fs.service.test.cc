#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/security/server_credentials.h>

#include "chord.types.h"
#include "chord.test.helper.h"
#include "util/chord.test.tmp.dir.h"
#include "util/chord.test.tmp.file.h"
#include "chord.node.h"
#include "chord.pb.h"
#include "chord.client.mock.h"
#include "chord.service.mock.h"
#include "chord_common.pb.h"
#include "chord.client.mock.h"
#include "chord.client.h"
#include "chord.log.h"
#include "chord_mock.grpc.pb.h"
#include "chord.router.h"
#include "chord.service.h"
#include "chord.fs.service.h"
#include "chord.facade.h"
#include "chord.fs.facade.h"
#include "chord.client.mock.h"
#include "chord.service.mock.h"
#include "chord.fs.metadata.manager.mock.h"

using chord::test::TmpDir;
using chord::test::TmpFile;

using chord::common::Header;
using chord::common::RouterEntry;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpc::InsecureServerCredentials;

using chord::fs::PutRequest;
using chord::fs::PutResponse;
using chord::fs::GetRequest;
using chord::fs::GetResponse;

using ::testing::StrictMock;
using ::testing::Eq;
using ::testing::_;
using ::testing::Return;

using namespace chord;
using namespace chord::test::helper;

class FsServiceTest : public ::testing::Test {
  protected:
    void SetUp() override {
      context = make_context(5, data_directory, meta_directory);
      router = new chord::Router(context);
      client = new MockClient();
      service = new MockService();
      chord_facade = std::make_unique<chord::ChordFacade>(context, router, client, service);
      //--- fs
      metadata_mgr = new chord::fs::MockMetadataManager;
      fs_service = new fs::Service(context, chord_facade.get(), metadata_mgr);
      fs_client = new fs::Client(context, chord_facade.get());
      fs_facade = std::make_unique<chord::fs::Facade>(context, fs_client, fs_service);

      ServerBuilder builder;
      builder.AddListeningPort("0.0.0.0:50050", InsecureServerCredentials());
      builder.RegisterService(fs_service);
      server = builder.BuildAndStart();
    }

    void TearDown() override {
      server->Shutdown();
    }

    //--- chord
    chord::Context context;
    chord::Router* router;
    chord::MockClient* client;
    chord::MockService* service;
    //--- fs
    chord::fs::IMetadataManager* metadata_mgr;
    std::unique_ptr<chord::ChordFacade> chord_facade;


    chord::fs::Service* fs_service;
    chord::fs::Client* fs_client;
    std::unique_ptr<chord::fs::Facade> fs_facade;
    std::unique_ptr<Server> server;

    // directories
    TmpDir meta_directory;
    TmpDir data_directory;
};

TEST_F(FsServiceTest, get) {
  GetRequest req;
  GetResponse res;
  ServerContext serverContext;

  RouterEntry entry;
  entry.set_endpoint("0.0.0.0:50050");
  entry.set_uuid("0");
  EXPECT_CALL(*service, successor(_)).WillOnce(Return(make_entry("0", "0.0.0.0:50050")));

  TmpDir tmp;
  TmpFile source_file(context.data_directory / "file");
  TmpFile target_file(tmp.path / "received_file");
  const auto status = fs_client->get(uri("chord:///file"), target_file.path);

  ASSERT_TRUE(status.ok());
  ASSERT_TRUE(chord::file::files_equal(source_file.path, target_file.path));
}

