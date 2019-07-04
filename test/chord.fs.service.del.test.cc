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
using ::testing::Ne;
using ::testing::_;
using ::testing::Return;

using namespace chord;
using namespace chord::fs;
using namespace chord::test;
using namespace chord::common;
using namespace chord::test;


class FilesystemServiceDelTest : public ::testing::Test {
  protected:
    void SetUp() override {
      self = make_unique<MockPeer>("0.0.0.0:50050", data_directory);
    }

    TmpDir data_directory;
    unique_ptr<MockPeer> self;
};

TEST_F(FilesystemServiceDelTest, del_file) {
  GetRequest req;
  GetResponse res;
  ServerContext serverContext;

  const auto recursive = false;
  const auto root_uri = uri("chord:///");
  const auto target_uri = uri("chord:///file");

  EXPECT_CALL(*self->service, successor(_))
    .WillOnce(Return(make_entry(self->context.node())))  // client
    .WillOnce(Return(make_entry(self->context.node())))  // initial delete - flag (handle_del_file)
    .WillOnce(Return(make_entry(self->context.node())))  // remove empty folder
    .WillOnce(Return(make_entry(self->context.node())))  // initial delete - flag (handle_del_dir)
    .WillOnce(Return(make_entry(self->context.node()))); // forward delete until not_found

  // delete file
  EXPECT_CALL(*self->metadata_mgr, exists(target_uri))
    .WillOnce(Return(true));

  Metadata metadata("file", "", "", perms::all, type::regular, {}, {}, {});
  EXPECT_CALL(*self->metadata_mgr, get(target_uri))
    .WillOnce(Return(std::set<Metadata>{metadata}));

  EXPECT_CALL(*self->metadata_mgr, del(target_uri))
    .WillOnce(Return(std::set<Metadata>{metadata}));

  // delete folder
  Metadata metadata_dir(".", "", "", perms::none, type::directory, {}, {}, {});
  EXPECT_CALL(*self->metadata_mgr, get(root_uri))
    .WillOnce(Return(std::set<Metadata>{metadata_dir}))  // within del
    .WillOnce(Return(std::set<Metadata>{metadata_dir}))  // within handle_del_dir
    .WillOnce(Return(std::set<Metadata>())); // forwarded delete -> not_found
  EXPECT_CALL(*self->metadata_mgr, del(root_uri))
    .WillOnce(Return(std::set<Metadata>{metadata}));

  // initial delete - flag
  //EXPECT_CALL(*self->service, successor(_))
  //  .WillOnce(Return(make_entry(self->context.node())));

  TmpFile target_file(self->context.data_directory / "file");
  ASSERT_TRUE(chord::file::exists(target_file.path));
  ASSERT_TRUE(chord::file::exists(self->context.data_directory));

  const auto status = self->fs_client->del(target_uri, recursive);

  ASSERT_TRUE(status.ok());
  ASSERT_FALSE(chord::file::exists(target_file.path));
  ASSERT_FALSE(chord::file::exists(self->context.data_directory));
}

//TEST_F(FilesystemServiceGetTest, get_from_node_reference) {
//  TmpDir source_data_directory;
//  const endpoint source_endpoint("0.0.0.0:50051");
//  MockPeer source_peer(source_endpoint, source_data_directory);
//
//  const auto source_uri = uri("chord:///file");
//  const auto source_file = source_data_directory.add_file("file");
//  const auto source_file_backup = source_data_directory.path / "file.backup";
//  file::copy_file(source_file.path, source_file_backup);
//
//  // connect the two nodes, first successor call will return self
//  EXPECT_CALL(*self->service, successor(_))
//    .WillOnce(Return(make_entry(self->context.node())))
//    .WillRepeatedly(Return(make_entry(source_peer.context.node())));
//  EXPECT_CALL(*source_peer.service, successor(_))
//    .WillRepeatedly(Return(make_entry(self->context.node())));
//
//  // reference source peer
//  Metadata metadata("file", "", "", perms::all, type::regular, {}, source_peer.context.node());
//  EXPECT_CALL(*self->metadata_mgr, get(source_uri))
//    .WillOnce(Return(std::set<Metadata>{metadata}));
//
//  // delete the node_ref and update the file hash
//  metadata = {"file", "", "", perms::all, type::regular, crypto::sha256(source_file.path), {}};
//  // update node_ref - file has been downloaded
//  std::set<Metadata> metadata_set{metadata};
//  EXPECT_CALL(*self->metadata_mgr, add(source_uri, metadata_set))
//    .WillOnce(Return(true));
//
//  // no metadata exists on the node referenced by node_ref
//  EXPECT_CALL(*source_peer.metadata_mgr, exists(source_uri))
//    .WillOnce(Return(false));
//
//  TmpDir target_directory;
//  const auto target_file = target_directory.path / "received_file";
//  const auto status = self->fs_client->get(source_uri, target_file);
//
//  ASSERT_TRUE(status.ok());
//  ASSERT_TRUE(chord::file::file_size(target_file) > 0);
//  ASSERT_TRUE(chord::file::files_equal(source_file_backup, target_file));
//
//  // original file has been removed
//  ASSERT_FALSE(chord::file::exists(source_file.path));
//}
//
//TEST_F(FilesystemServiceGetTest, get_from_replication) {
//  TmpDir source_data_directory;
//  const endpoint source_endpoint("0.0.0.0:50051");
//  MockPeer source_peer(source_endpoint, source_data_directory);
//
//  const auto source_uri = uri("chord:///file");
//  const auto source_file = source_data_directory.add_file("file");
//
//  // connect the two nodes, first successor call will return self
//  EXPECT_CALL(*self->service, successor(_))
//    .WillOnce(Return(make_entry(self->context.node())))
//    .WillRepeatedly(Return(make_entry(source_peer.context.node())));
//  EXPECT_CALL(*source_peer.service, successor(_))
//    .WillRepeatedly(Return(make_entry(self->context.node())));
//
//  // reference source peer
//  Metadata metadata("file", "", "", perms::all, type::regular, {}, {}, Replication(2));
//  EXPECT_CALL(*self->metadata_mgr, get(source_uri))
//    .WillOnce(Return(std::set<Metadata>{metadata}));
//
//  TmpDir target_directory;
//  const auto target_file = target_directory.path / "received_file";
//  const auto status = self->fs_client->get(source_uri, target_file);
//
//  ASSERT_TRUE(status.ok());
//  ASSERT_TRUE(chord::file::file_size(target_file) > 0);
//  ASSERT_TRUE(chord::file::files_equal(source_file.path, target_file));
//  // copied file to local data_directory
//  ASSERT_TRUE(chord::file::files_equal(source_file.path, self->context.data_directory / "file"));
//}
//
//TEST_F(FilesystemServiceGetTest, get_from_replication_propagates) {
//  // peer holding the last replication
//  TmpDir source_data_directory;
//  const endpoint source_endpoint("0.0.0.0:50052");
//  MockPeer source_peer(source_endpoint, source_data_directory);
//
//  // peer just to hop-over
//  TmpDir middle_data_directory;
//  const endpoint middle_endpoint("0.0.0.0:50051");
//  MockPeer middle_peer(middle_endpoint, middle_data_directory);
//
//  const auto source_uri = uri("chord:///file");
//  const auto source_file = source_data_directory.add_file("file");
//
//  // connect the two nodes, first successor call will return self
//  EXPECT_CALL(*self->service, successor(_))
//    .WillOnce(Return(make_entry(self->context.node())))
//    .WillRepeatedly(Return(make_entry(middle_peer.context.node())));
//  EXPECT_CALL(*middle_peer.service, successor(_))
//    .WillRepeatedly(Return(make_entry(source_peer.context.node())));
//  EXPECT_CALL(*source_peer.service, successor(_))
//    .WillRepeatedly(Return(make_entry(self->context.node())));
//
//  // self peer metadata
//  Metadata metadata("file", "", "", perms::all, type::regular, {}, {}, Replication(0,2));
//  EXPECT_CALL(*self->metadata_mgr, get(source_uri))
//    .WillOnce(Return(std::set<Metadata>{metadata}));
//
//  metadata.replication = {1,2};
//  EXPECT_CALL(*middle_peer.metadata_mgr, get(source_uri))
//    .WillOnce(Return(std::set<Metadata>{metadata}));
//
//  TmpDir target_directory;
//  const auto target_file = target_directory.path / "received_file";
//  const auto status = self->fs_client->get(source_uri, target_file);
//
//  ASSERT_TRUE(status.ok());
//  ASSERT_TRUE(chord::file::file_size(target_file) > 0);
//  ASSERT_TRUE(chord::file::files_equal(source_file.path, target_file));
//  // copied file to local data_directories (middle/self)
//  ASSERT_TRUE(chord::file::files_equal(source_file.path, self->context.data_directory / "file"));
//  ASSERT_TRUE(chord::file::files_equal(source_file.path, middle_peer.context.data_directory / "file"));
//}
