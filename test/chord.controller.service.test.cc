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

class ControllerServiceTest : public ::testing::Test {
  protected:
    void SetUp() override {
      ctxt = make_context(5);
      router = new chord::Router(ctxt);
      client = new chord::Client(ctxt, router);
      service= new chord::Service(ctxt, router, client);
      chord_facade = std::make_unique<chord::ChordFacade>(ctxt, router, client, service);
      ctrl_service = std::make_unique<chord::controller::Service>(ctxt, &fs_facade);
    }

    chord::Context ctxt;
    chord::Router* router;
    chord::Client* client;
    chord::Service* service;
    std::unique_ptr<chord::ChordFacade> chord_facade;
    std::unique_ptr<chord::controller::Service> ctrl_service;
    StrictMock<chord::fs::MockFacade> fs_facade;

    ControlRequest req;
    ControlResponse res;
};

TEST_F(ControllerServiceTest, empty_request) {
  req.set_command("");
  const auto status = ctrl_service->control(nullptr, &req, &res);
  ASSERT_FALSE(status.ok());
}

TEST_F(ControllerServiceTest, put_request) {
  req.set_command("put --repl 2 /tmp/first ~/workspace/chord/test_data/folder/ chord:///");

  EXPECT_CALL(fs_facade, put(Eq(path{"/tmp/first"}), Eq(uri{"chord:///"}), Eq(Replication{2})));
  EXPECT_CALL(fs_facade, put(Eq(path{"~/workspace/chord/test_data/folder/"}), Eq(uri{"chord:///"}), Eq(Replication{2})));

  const auto status = ctrl_service->control(nullptr, &req, &res);
  ASSERT_TRUE(status.ok());
}

