#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <memory>
#include <set>
#include <string>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/security/server_credentials.h>

#include "chord_fs.pb.h"
#include "chord.optional.h"
#include "chord.client.mock.h"
#include "chord.context.h"
#include "chord.facade.h"
#include "chord.file.h"
#include "chord.fs.client.h"
#include "chord.fs.facade.h"
#include "chord.fs.metadata.h"
#include "chord.fs.metadata.manager.mock.h"
#include "chord.fs.perms.h"
#include "chord.fs.replication.h"
#include "chord.fs.service.h"
#include "chord.fs.type.h"
#include "chord.node.h"
#include "chord.path.h"
#include "chord.router.h"
#include "chord.service.mock.h"
#include "chord.test.helper.h"
#include "chord.types.h"
#include "chord.uri.h"
#include "chord.uuid.h"
#include "chord_common.pb.h"
#include "util/chord.test.tmp.dir.h"
#include "util/chord.test.tmp.file.h"

using std::make_unique;
using std::unique_ptr;

using chord::common::Header;
using chord::common::RouterEntry;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpc::InsecureServerCredentials;

using ::testing::StrictMock;
using ::testing::Eq;
using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;

using namespace chord;
using namespace chord::fs;
using namespace chord::test;
using namespace chord::common;
using namespace chord::test::helper;

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
      fs_client = new fs::Client(context, chord_facade.get());
      fs_facade = std::make_unique<chord::fs::Facade>(context, fs_client, fs_service);

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

class FsServiceTest : public ::testing::Test {
  protected:
    void SetUp() override {
      self = make_unique<MockPeer>("0.0.0.0:50050", data_directory);
    }

    TmpDir data_directory;
    unique_ptr<MockPeer> self;
};

TEST_F(FsServiceTest, get) {
  GetRequest req;
  GetResponse res;
  ServerContext serverContext;

  EXPECT_CALL(*self->service, successor(_)).WillOnce(Return(make_entry("0", "0.0.0.0:50050")));

  TmpDir tmp;
  TmpFile source_file(self->context.data_directory / "file");
  TmpFile target_file(tmp.path / "received_file");
  const auto status = self->fs_client->get(uri("chord:///file"), target_file.path);

  ASSERT_TRUE(status.ok());
  ASSERT_TRUE(chord::file::file_size(target_file.path) > 0);
  ASSERT_TRUE(chord::file::files_equal(source_file.path, target_file.path));
}

TEST_F(FsServiceTest, get_from_node_reference) {
  TmpDir source_data_directory;
  const endpoint source_endpoint("0.0.0.0:50051");
  MockPeer source_peer(source_endpoint, source_data_directory);

  const auto source_uri = uri("chord:///file");
  const auto source_file = source_data_directory.add_file("file");
  const auto source_file_backup = source_data_directory.path / "file.backup";
  file::copy_file(source_file.path, source_file_backup);

  // connect the two nodes, first successor call will return self
  EXPECT_CALL(*self->service, successor(_))
    .WillOnce(Return(make_entry(self->context.node())))
    .WillRepeatedly(Return(make_entry(source_peer.context.node())));
  EXPECT_CALL(*source_peer.service, successor(_))
    .WillRepeatedly(Return(make_entry(self->context.node())));

  // reference source peer
  Metadata metadata("file", "", "", perms::all, type::regular, source_peer.context.node());
  EXPECT_CALL(*self->metadata_mgr, get(source_uri))
    .WillOnce(Return(std::set<Metadata>{metadata}));

  // after downloading the file will be deleted
  metadata = {"file", "", "", perms::all, type::regular};
  EXPECT_CALL(*source_peer.metadata_mgr, get(source_uri))
    .WillOnce(Return(std::set<Metadata>{metadata}));
  EXPECT_CALL(*source_peer.metadata_mgr, del(source_uri))
    .WillOnce(Return(std::set<Metadata>{metadata}));

  TmpDir target_directory;
  const auto target_file = target_directory.path / "received_file";
  const auto status = self->fs_client->get(source_uri, target_file);

  ASSERT_TRUE(status.ok());
  ASSERT_TRUE(chord::file::file_size(target_file) > 0);
  ASSERT_TRUE(chord::file::files_equal(source_file_backup, target_file));

  // original file has been removed
  ASSERT_FALSE(chord::file::exists(source_file.path));
}

TEST_F(FsServiceTest, get_from_replication) {
  TmpDir source_data_directory;
  const endpoint source_endpoint("0.0.0.0:50051");
  MockPeer source_peer(source_endpoint, source_data_directory);

  const auto source_uri = uri("chord:///file");
  const auto source_file = source_data_directory.add_file("file");

  // connect the two nodes, first successor call will return self
  EXPECT_CALL(*self->service, successor(_))
    .WillOnce(Return(make_entry(self->context.node())))
    .WillRepeatedly(Return(make_entry(source_peer.context.node())));
  EXPECT_CALL(*source_peer.service, successor(_))
    .WillRepeatedly(Return(make_entry(self->context.node())));

  // reference source peer
  Metadata metadata("file", "", "", perms::all, type::regular, {}, Replication(2));
  EXPECT_CALL(*self->metadata_mgr, get(source_uri))
    .WillOnce(Return(std::set<Metadata>{metadata}));

  TmpDir target_directory;
  const auto target_file = target_directory.path / "received_file";
  const auto status = self->fs_client->get(source_uri, target_file);

  ASSERT_TRUE(status.ok());
  ASSERT_TRUE(chord::file::file_size(target_file) > 0);
  ASSERT_TRUE(chord::file::files_equal(source_file.path, target_file));
  // copied file to local data_directory
  ASSERT_TRUE(chord::file::files_equal(source_file.path, self->context.data_directory / "file"));
}

TEST_F(FsServiceTest, get_from_replication_propagates) {
  // peer holding the last replication
  TmpDir source_data_directory;
  const endpoint source_endpoint("0.0.0.0:50052");
  MockPeer source_peer(source_endpoint, source_data_directory);

  // peer just to hop-over
  TmpDir middle_data_directory;
  const endpoint middle_endpoint("0.0.0.0:50051");
  MockPeer middle_peer(middle_endpoint, middle_data_directory);

  const auto source_uri = uri("chord:///file");
  const auto source_file = source_data_directory.add_file("file");

  // connect the two nodes, first successor call will return self
  EXPECT_CALL(*self->service, successor(_))
    .WillOnce(Return(make_entry(self->context.node())))
    .WillRepeatedly(Return(make_entry(middle_peer.context.node())));
  EXPECT_CALL(*middle_peer.service, successor(_))
    .WillRepeatedly(Return(make_entry(source_peer.context.node())));
  EXPECT_CALL(*source_peer.service, successor(_))
    .WillRepeatedly(Return(make_entry(self->context.node())));

  // self peer metadata
  Metadata metadata("file", "", "", perms::all, type::regular, {}, Replication(0,2));
  EXPECT_CALL(*self->metadata_mgr, get(source_uri))
    .WillOnce(Return(std::set<Metadata>{metadata}));

  metadata.replication = {1,2};
  EXPECT_CALL(*middle_peer.metadata_mgr, get(source_uri))
    .WillOnce(Return(std::set<Metadata>{metadata}));

  TmpDir target_directory;
  const auto target_file = target_directory.path / "received_file";
  const auto status = self->fs_client->get(source_uri, target_file);

  ASSERT_TRUE(status.ok());
  ASSERT_TRUE(chord::file::file_size(target_file) > 0);
  ASSERT_TRUE(chord::file::files_equal(source_file.path, target_file));
  // copied file to local data_directories (middle/self)
  ASSERT_TRUE(chord::file::files_equal(source_file.path, self->context.data_directory / "file"));
  ASSERT_TRUE(chord::file::files_equal(source_file.path, middle_peer.context.data_directory / "file"));
}
