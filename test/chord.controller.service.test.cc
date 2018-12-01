#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

#include "chord.test.helper.h"

#include "chord.context.h"
#include "chord.path.h"
#include "chord.uri.h"
#include "chord.fs.replication.h"
#include "chord.controller.service.h"
#include "chord.facade.h"
#include "chord.fs.facade.h"
#include "chord.fs.facade.mock.h"
#include "chord.pb.h"
#include "chord_fs.pb.h"
#include "chord_mock.grpc.pb.h"

using namespace std;
using namespace chord::controller;
using namespace chord::test::helper;

using chord::path;
using chord::uri;
using chord::Context;
using chord::Router;
using chord::Client;
using chord::Service;
using grpc::Status;
using chord::fs::Replication;

using ::testing::Return;
using ::testing::_;
using ::testing::Invoke;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::SetArgPointee;
using ::testing::DoAll;
using ::testing::StrictMock;
using ::testing::Eq;


TEST(chord_controller_service, constructor) {
  auto ctxt = make_context(5);
  chord::ChordFacade chord_facade{ctxt};
  chord::fs::Facade fs_facade{ctxt, &chord_facade};
  chord::controller::Service service{ctxt, &fs_facade};
}

TEST(chord_controller_service, empty_request) {
  chord::MockChordStub stub;
  auto ctxt = make_context(5);
  auto router = new chord::Router{ctxt};
  auto client = new chord::Client{ctxt, router};
  auto service = new chord::Service{ctxt, router, client};
  chord::ChordFacade chord_facade{ctxt, router, client, service};
  StrictMock<chord::fs::MockFacade> fs_facade;
  chord::controller::Service ctrl_service{ctxt, &fs_facade};

  ControlRequest req;
  ControlResponse res;

  req.set_command("");
  const auto status = ctrl_service.control(nullptr, &req, &res);
  ASSERT_FALSE(status.ok());
}

TEST(chord_controller_service, put_request) {
  auto ctxt = make_context(5);
  chord::ChordFacade chord_facade{ctxt};
  chord::fs::MockFacade fs_facade;
  chord::controller::Service ctrl_service{ctxt, &fs_facade};

  ControlRequest req;
  ControlResponse res;

  EXPECT_CALL(fs_facade, put(Eq(path{"/tmp/first"}), Eq(uri{"chord:///"}), Eq(Replication{2})));
  EXPECT_CALL(fs_facade, put(Eq(path{"~/workspace/chord/test_data/folder/"}), Eq(uri{"chord:///"}), Eq(Replication{2})));

  req.set_command("put --repl 2 /tmp/first ~/workspace/chord/test_data/folder/ chord:///");
  const auto status = ctrl_service.control(nullptr, &req, &res);
  ASSERT_TRUE(status.ok());
}
