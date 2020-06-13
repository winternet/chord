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
    .WillOnce(Return(make_entry(self->context.node())))  // client - remove file
    .WillOnce(Return(make_entry(self->context.node())))  // initial delete - flag (handle_del_file)
    .WillOnce(Return(make_entry(self->context.node())))  // client - remove empty folder
    .WillOnce(Return(make_entry(self->context.node()))); // initial delete - flag (handle_del_dir)
    //.WillOnce(Return(make_entry(self->context.node()))); // forward delete until not_found

  //--- delete file
  //EXPECT_CALL(*self->metadata_mgr, exists(target_uri))
  //  .WillOnce(Return(true));

  Metadata metadata("file", "", "", perms::all, type::regular, {}, {}, {});
  EXPECT_CALL(*self->metadata_mgr, get(target_uri))
    .WillOnce(Return(std::set<Metadata>{metadata}));
  EXPECT_CALL(*self->metadata_mgr, exists(target_uri))
    .WillOnce(Return(true));
  EXPECT_CALL(*self->metadata_mgr, del(target_uri))
    .WillOnce(Return(std::set<Metadata>{metadata}));

  //--- delete folder
  Metadata metadata_dir(".", "", "", perms::none, type::directory, {}, {}, {});
  EXPECT_CALL(*self->metadata_mgr, get(root_uri))
    .WillOnce(Return(std::set<Metadata>{metadata_dir}))  // within del
    .WillOnce(Return(std::set<Metadata>{metadata_dir})); // within del_handle_recursive
    //.WillOnce(Return(std::set<Metadata>{metadata_dir}))  // within handle_del_dir
    //.WillOnce(Return(std::set<Metadata>()));             // forwarded delete -> not_found
  EXPECT_CALL(*self->metadata_mgr, del(root_uri, std::set<Metadata>{metadata}, false))
    .WillOnce(Return(std::set<Metadata>{}));
  EXPECT_CALL(*self->metadata_mgr, del(root_uri))
    .WillOnce(Return(std::set<Metadata>{metadata}));

  TmpFile target_file(self->context.data_directory / "file");
  ASSERT_TRUE(chord::file::exists(target_file.path));
  ASSERT_TRUE(chord::file::exists(self->context.data_directory));

  const auto status = self->fs_client->del(target_uri, recursive);

  ASSERT_TRUE(status.ok());
  ASSERT_FALSE(chord::file::exists(target_file.path));
  ASSERT_FALSE(chord::file::exists(self->context.data_directory));
}
