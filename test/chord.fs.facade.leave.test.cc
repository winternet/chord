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
      self = make_unique<MockPeer>("0.0.0.0:50050", data_directory);
    }

    TmpDir data_directory;
    unique_ptr<MockPeer> self;
};

TEST_F(FilesystemFacadeLeaveTest, on_leave__handle_node_reference) {

  TmpDir data_directory_2;
  const endpoint source_endpoint_2("0.0.0.0:50051");
  MockPeer peer_2(source_endpoint_2, data_directory_2);

  TmpDir source_directory;

  const auto target_uri = uri("chord:///file");
  const auto source_file = source_directory.add_file("file");

  map<uri, set<Metadata>> empty;
  EXPECT_CALL(*peer_2.metadata_mgr, get(self->context.uuid(), peer_2.context.uuid()))
    .WillOnce(Return(empty));

  //TODO add test for directory handling
  map<uri, set<Metadata>> shallow_copies;
  Metadata metadata_file("file", "", "", perms::all, type::regular, crypto::sha256(source_file.path), {}, Replication());
  set<Metadata> metadata_set{metadata_file};
  shallow_copies[target_uri] = metadata_set;

  EXPECT_CALL(*peer_2.metadata_mgr, get_shallow_copies(peer_2.context.node()))
    .WillOnce(Return(shallow_copies));
  EXPECT_CALL(*peer_2.metadata_mgr, del(target_uri))
    .WillOnce(Return(metadata_set));
  EXPECT_CALL(*self->metadata_mgr, add(target_uri, metadata_set))
    .WillOnce(Return(true));

  peer_2.fs_facade->on_leave(self->context.node(), self->context.node());
}
