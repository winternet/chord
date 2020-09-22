#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "util/chord.test.helper.h"

#include "chord.client.h"                // for Client
#include "chord.router.h"                // for Router
#include "chord.service.h"               // for Service
#include "chord.channel.pool.h"
#include "chord.context.h"
#include "chord.path.h"
#include "chord.uri.h"
#include "chord.fs.replication.h"
#include "chord.controller.service.h"
#include "chord.facade.h"
#include "chord.fs.facade.mock.h"

using namespace std;
using namespace chord::controller;
using namespace chord::test;

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
using ::testing::NiceMock;
using ::testing::Eq;

class ControllerServiceTest : public ::testing::Test {
  protected:
    void SetUp() override {
      ctxt = make_context(5);
      router = new chord::Router(ctxt);
      channel_pool = std::make_unique<chord::ChannelPool>(ctxt);
      client = new chord::Client(ctxt, router, channel_pool.get());
      service= new chord::Service(ctxt, router, client);
      chord_facade = std::make_unique<chord::ChordFacade>(ctxt, router, client, service);
      ctrl_service = std::make_unique<chord::controller::Service>(ctxt, &fs_facade);
    }

    chord::Context ctxt;
    chord::Router* router;
    std::unique_ptr<chord::ChannelPool> channel_pool;
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

TEST_F(ControllerServiceTest, unknown_command_request) {
  req.set_command("nonmapped /first chord:///");
  const auto status = ctrl_service->control(nullptr, &req, &res);
  ASSERT_FALSE(status.ok());
}

TEST_F(ControllerServiceTest, put_request) {
  req.set_command("put --repl 2 /first /second chord:///");

  EXPECT_CALL(fs_facade, put(Eq(path{"/first"}), Eq(uri{"chord:///"}), Eq(Replication{2})));
  EXPECT_CALL(fs_facade, put(Eq(path{"/second"}), Eq(uri{"chord:///"}), Eq(Replication{2})));

  const auto status = ctrl_service->control(nullptr, &req, &res);
  ASSERT_TRUE(status.ok());
}

TEST_F(ControllerServiceTest, get_empty_request) {
  req.set_command("get ");
  const auto status = ctrl_service->control(nullptr, &req, &res);
  ASSERT_FALSE(status.ok());
}

TEST_F(ControllerServiceTest, get_request) {
  req.set_command("get chord:///first /tmp");

  EXPECT_CALL(fs_facade, get(Eq(uri{"chord:///first"}), Eq(path{"/tmp"})));

  const auto status = ctrl_service->control(nullptr, &req, &res);
  ASSERT_TRUE(status.ok());
}

TEST_F(ControllerServiceTest, dir_empty_request) {
  req.set_command("dir ");
  const auto status = ctrl_service->control(nullptr, &req, &res);
  ASSERT_FALSE(status.ok());
}

TEST_F(ControllerServiceTest, dir_request) {
  req.set_command("dir chord:///first");

  EXPECT_CALL(fs_facade, dir(Eq(uri{"chord:///first"}), _));

  const auto status = ctrl_service->control(nullptr, &req, &res);
  ASSERT_TRUE(status.ok());
}

TEST_F(ControllerServiceTest, del_empty_request) {
  req.set_command("del ");
  const auto status = ctrl_service->control(nullptr, &req, &res);
  ASSERT_FALSE(status.ok());
}

TEST_F(ControllerServiceTest, del_request) {
  req.set_command("del chord:///first");

  EXPECT_CALL(fs_facade, del(Eq(uri{"chord:///first"}), Eq(false)));

  const auto status = ctrl_service->control(nullptr, &req, &res);
  ASSERT_TRUE(status.ok());
}

TEST_F(ControllerServiceTest, del_request__recursive) {
  req.set_command("del -r chord:///first");

  EXPECT_CALL(fs_facade, del(Eq(uri{"chord:///first"}), Eq(true)));

  const auto status = ctrl_service->control(nullptr, &req, &res);
  ASSERT_TRUE(status.ok());
}
