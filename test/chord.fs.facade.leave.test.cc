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
using std::map;
using std::set;

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

class FilesystemFacadeLeaveTest : public ::testing::Test {
  protected:
    void SetUp() override {
      self = make_unique<MockPeer>("0.0.0.0:50050", make_shared<TmpDir>());
    }

    unique_ptr<MockPeer> self;
};

// self leaves the cluster
TEST_F(FilesystemFacadeLeaveTest, on_leave__handle_local_files) {

  MockPeer peer_2(endpoint{"0.0.0.0:50051"}, make_shared<TmpDir>());

  const auto source_directory = std::make_shared<TmpDir>();

  const auto target_uri = uri("chord:///file");
  const auto source_file = self->data_directory->add_file("file");

  //map<uri, set<Metadata>> files;
  IMetadataManager::uri_meta_map_desc files;
  Metadata metadata_file("file", perms::all, file::file_size(source_file->path), crypto::sha256(source_file->path), {}, Replication());
  set<Metadata> metadata_set{metadata_file};
  files[target_uri] = metadata_set;

  // take self out of the ring, peer_2's successor is peer_2 (self 'already' left)
  EXPECT_CALL(*peer_2.service, successor(_))
    .WillRepeatedly(Return(make_entry(peer_2.context.node())));
  EXPECT_CALL(*self->service, successor(_))
    .WillRepeatedly(Return(make_entry(peer_2.context.node())));

  EXPECT_CALL(*self->metadata_mgr, exists(_))
    .WillRepeatedly(Return(true));
  //EXPECT_CALL(*self->metadata_mgr, get(_))
  //  .WillOnce(Return(metadata_set));
  EXPECT_CALL(*self->metadata_mgr, get_all())
    .WillOnce(Return(files));

  // peer2 adds metadata for file & directory
  EXPECT_CALL(*peer_2.metadata_mgr, add(target_uri, metadata_set))
    .WillOnce(Return(true));
  EXPECT_CALL(*peer_2.metadata_mgr, add(uri(target_uri.scheme(), target_uri.path().parent_path()), _))
    .WillOnce(Return(true));
  // peer2 doesnt have the file yet
  EXPECT_CALL(*peer_2.metadata_mgr, exists(_))
    .WillRepeatedly(Return(false));

  self->fs_facade->on_leave(peer_2.context.node(), peer_2.context.node());

  const auto target_file = peer_2.context.data_directory / target_uri.path().filename();
  ASSERT_TRUE(chord::file::file_size(target_file) > 0);
  ASSERT_TRUE(chord::file::files_equal(source_file->path, target_file));
}

//peer2 leaves the cluster
TEST_F(FilesystemFacadeLeaveTest, on_leave__handle_node_reference) {

  MockPeer peer_2(endpoint{"0.0.0.0:50051"}, make_shared<TmpDir>());
  const auto source_directory = std::make_shared<TmpDir>();

  const auto root_uri = uri("chord:///");
  const auto target_uri = uri("chord:///file");
  const auto source_file = source_directory->add_file("file");

  const auto successor = make_entry(self->context.node());
  EXPECT_CALL(*peer_2.service, successor(_))
    .WillRepeatedly(Return(successor));
  EXPECT_CALL(*peer_2.client, successor(_))
    .WillRepeatedly(Return(successor));

  //TODO add test for directory handling
  IMetadataManager::uri_meta_map_desc shallow_copies;
  //map<uri, set<Metadata>> shallow_copies;
  Metadata metadata_dir(".", "", "", perms::none, type::directory, 0,crypto::sha256(source_file->path.parent_path()), {}, Replication());
  Metadata metadata_file("file", perms::all, file::file_size(source_file->path), crypto::sha256(source_file->path), self->context.node(), Replication());

  shallow_copies[target_uri] = {metadata_file};
  shallow_copies[root_uri] = {metadata_dir, metadata_file};

  EXPECT_CALL(*peer_2.metadata_mgr, get_all())
    .WillOnce(Return(shallow_copies));
  EXPECT_CALL(*peer_2.metadata_mgr, del(target_uri))
    .WillOnce(Return(std::set<Metadata>{metadata_file}));
  EXPECT_CALL(*peer_2.metadata_mgr, del(root_uri))
    .WillOnce(Return(std::set<Metadata>{metadata_dir, metadata_file}));

  EXPECT_CALL(*self->metadata_mgr, add(target_uri, std::set<Metadata>{metadata_file}))
    .WillOnce(Return(true));
  EXPECT_CALL(*self->metadata_mgr, add(root_uri, std::set<Metadata>{metadata_dir, metadata_file}))
    .WillOnce(Return(true));

  peer_2.fs_facade->on_leave(self->context.node(), self->context.node());
}
