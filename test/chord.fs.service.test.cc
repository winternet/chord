#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <functional>

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
#include "chord.fs.metadata.h"
#include "chord.optional.h"

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

using chord::fs::Metadata;

using chord::fs::PutRequest;
using chord::fs::PutResponse;
using chord::fs::GetRequest;
using chord::fs::GetResponse;

using ::testing::StrictMock;
using ::testing::Eq;
using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;

using namespace chord;
using namespace chord::fs;
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
    chord::fs::MockMetadataManager* metadata_mgr;
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

  EXPECT_CALL(*service, successor(_)).WillOnce(Return(make_entry("0", "0.0.0.0:50050")));

  TmpDir tmp;
  TmpFile source_file(context.data_directory / "file");
  TmpFile target_file(tmp.path / "received_file");
  const auto status = fs_client->get(uri("chord:///file"), target_file.path);

  ASSERT_TRUE(status.ok());
  ASSERT_TRUE(chord::file::files_equal(source_file.path, target_file.path));
}

/**
 * Called to get the file from the referenced node.
 *
 * Since we intentionally did not create the file for the first
 * run, we need
 */
RouterEntry handle_get_from_node_reference(const chord::uri& source_uri, const chord::Context& context) {
  const auto generated_file = context.data_directory / source_uri.path();

  // deleted by TmpDir dtor
  std::ofstream file(generated_file);
  file << chord::uuid::random().string();
  file.close();

  return make_entry("0", "0.0.0.0:50050");
}

/**
 * The first time the get is called the file does not exist but the
 * metadata is referencing the same node (just for testing reasons).
 * During the second call to self, the file is created within
 * handle_get_from_node_reference().
 */
TEST_F(FsServiceTest, get_from_node_reference) {
  GetRequest req;
  GetResponse res;
  ServerContext serverContext;

  const auto source_uri = uri("chord:///file");
  auto handler = std::bind(handle_get_from_node_reference, std::cref(source_uri), std::cref(context));

  EXPECT_CALL(*service, successor(_))
    .WillOnce(Return(make_entry("0", "0.0.0.0:50050")))  // initial
    .WillOnce(InvokeWithoutArgs(handler)); // node ref - get create file to 'fake' accessing a different node

  // reference ourself
  Metadata metadata("file", "", "", perms::all, type::regular, chord::node{"0", "0.0.0.0:50050"});
  EXPECT_CALL(*metadata_mgr, get(source_uri))
    .WillRepeatedly(Return(std::set<Metadata>{metadata}));

  // note without node reference for deletion
  metadata = {"file", "", "", perms::all, type::regular};
  EXPECT_CALL(*metadata_mgr, del(source_uri))
    .WillOnce(Return(std::set<Metadata>{metadata}));

  TmpDir tmp;
  TmpFile target_file(tmp.path / "received_file");
  const auto status = fs_client->get(source_uri, target_file.path);

  ASSERT_TRUE(status.ok());
  ASSERT_TRUE(chord::file::files_equal(context.data_directory / source_uri.path(), target_file.path));
}
