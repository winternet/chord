#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "chord.peer.mock.h"

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
#include "chord.crypto.h"
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
#include "chord.types.h"
#include "chord.uri.h"
#include "chord.uuid.h"
#include "chord_common.pb.h"
#include "util/chord.test.tmp.dir.h"
#include "util/chord.test.tmp.file.h"
#include "util/chord.test.helper.h"

using std::make_unique;
using std::unique_ptr;
using std::make_shared;
using std::shared_ptr;

using chord::common::Header;
using chord::common::RouterEntry;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpc::InsecureServerCredentials;

using ::testing::Eq;
using ::testing::Ne;
using ::testing::_;
using ::testing::Return;

using namespace chord;
using namespace chord::fs;
using namespace chord::test;
using namespace chord::common;
using namespace chord::test;

class FilesystemServicePutTest : public ::testing::Test {
  protected:
    void SetUp() override {
      self = make_unique<MockPeer>("0.0.0.0:50050", make_shared<TmpDir>());
    }

    unique_ptr<MockPeer> self;
};

TEST_F(FilesystemServicePutTest, put) {
  TmpDir source_directory;

  const auto target_uri = uri("chord:///file");
  const auto source_file = source_directory.add_file("file");

  // connect the two nodes, first successor call will return self
  EXPECT_CALL(*self->service, successor(_))
    .WillRepeatedly(Return(make_entry(self->context.node())));

  // self peer metadata
  EXPECT_CALL(*self->metadata_mgr, exists(target_uri))
    .WillOnce(Return(false));

  Metadata metadata_file("file", "", "", perms::all, type::regular, file::file_size(source_file->path), crypto::sha256(source_file->path), {}, Replication());
  std::set<Metadata> metadata_set{metadata_file};
  EXPECT_CALL(*self->metadata_mgr, add(target_uri, metadata_set))
    .WillOnce(Return(true));

  Metadata metadata_dir = {".", "", "", perms::none, type::directory, 0, {}, {}, Replication()};
  std::set<Metadata> metadata_set_dir{metadata_dir, metadata_file};
  EXPECT_CALL(*self->metadata_mgr, add(uri(target_uri.scheme(), target_uri.path().parent_path()), metadata_set_dir))
    .WillOnce(Return(true));

  const auto status = self->fs_client->put(target_uri, *source_file, {});

  const auto target_file = self->data_directory->path / target_uri.path();
  ASSERT_TRUE(status.ok());
  ASSERT_TRUE(chord::file::file_size(target_file) > 0);
  ASSERT_TRUE(chord::file::files_equal(source_file->path, target_file));
}

TEST_F(FilesystemServicePutTest, put_replication_2) {
  TmpDir source_directory;

  const auto data_directory_2 = std::make_shared<TmpDir>();
  const endpoint source_endpoint_2("0.0.0.0:50051");
  MockPeer peer_2(source_endpoint_2, data_directory_2);

  const auto root_uri = uri("chord:///");
  const auto target_uri = uri("chord:///file");
  const auto source_file = source_directory.add_file("file");

  // setup networking
  {
    // stay on same node
    EXPECT_CALL(*self->service, successor(Ne(self->context.uuid())))
      .WillRepeatedly(Return(make_entry(self->context.node())));
    // replication handling will ask for own successor -> next node
    EXPECT_CALL(*self->service, successor(self->context.uuid()))
      .WillRepeatedly(Return(make_entry(peer_2.context.node())));

    // stay on same node
    EXPECT_CALL(*peer_2.service, successor(Ne(peer_2.context.uuid())))
      .WillRepeatedly(Return(make_entry(peer_2.context.node())));
    // replication handling will ask for own successor -> next node
    EXPECT_CALL(*peer_2.service, successor(peer_2.context.uuid()))
      .WillRepeatedly(Return(make_entry(self->context.node())));
  }

  // setup expectations for self
  {
    Metadata metadata_dir{".", "", "", perms::none, type::directory, 0, {}, {}, Replication(0,2)};
    Metadata metadata_file{"file", "", "", perms::all, type::regular, file::file_size(source_file->path), crypto::sha256(source_file->path), {}, Replication(0,2)};

    // self peer metadata
    EXPECT_CALL(*self->metadata_mgr, exists(target_uri))
      .WillOnce(Return(false)) // no metadata available
      .WillOnce(Return(true)); // metadata has been added meanwhile
    EXPECT_CALL(*self->metadata_mgr, get(target_uri))
      .WillOnce(Return(std::set<Metadata>{metadata_file})); // meanwhile added metadata

    EXPECT_CALL(*self->metadata_mgr, add(target_uri, std::set<Metadata>{metadata_file}))
      .WillOnce(Return(true));
    EXPECT_CALL(*self->metadata_mgr, add(root_uri, std::set<Metadata>{metadata_dir, metadata_file}))
      .WillOnce(Return(true));
  }


  // setup expectations for second-node
  {
  Metadata metadata_dir{".", "", "", perms::none, type::directory, 0, {}, {}, Replication(1,2)};
  Metadata metadata_file{"file", "", "", perms::all, type::regular, file::file_size(source_file->path), crypto::sha256(source_file->path), {}, Replication(1,2)};

  EXPECT_CALL(*peer_2.metadata_mgr, add(target_uri, std::set<Metadata>{metadata_file}))
    .WillOnce(Return(true));
  EXPECT_CALL(*peer_2.metadata_mgr, add(root_uri, std::set<Metadata>{metadata_dir, metadata_file}))
    .WillOnce(Return(true));
  }

  const auto status = self->fs_client->put(target_uri, *source_file, client::options{Replication(2)});

  ASSERT_TRUE(status.ok());
  const auto target_file = self->data_directory->path / target_uri.path();
  ASSERT_TRUE(chord::file::file_size(target_file) > 0);
  ASSERT_TRUE(chord::file::files_equal(source_file->path, target_file));
  const auto target_file_2 = peer_2.data_directory->path / target_uri.path();
  ASSERT_TRUE(chord::file::file_size(target_file_2) > 0);
  ASSERT_TRUE(chord::file::files_equal(source_file->path, target_file_2));
}
