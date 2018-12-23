#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "chord.test.helper.h"
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

using chord::common::Header;
using chord::common::RouterEntry;

using grpc::ServerContext;
using grpc::Status;

using chord::fs::PutRequest;
using chord::fs::PutResponse;

using ::testing::StrictMock;
using ::testing::Eq;

using namespace chord;
using namespace chord::test::helper;

class FsServiceTest : public ::testing::Test {
  protected:
    void SetUp() override {
      ctxt = make_context(5);
      router = new chord::Router(ctxt);
      client = new MockClient();
      service = new MockService();
      chord_facade = std::make_unique<chord::ChordFacade>(ctxt, router, client, service);
      //--- fs
      metadata_mgr = new chord::fs::MockMetadataManager;
      fs_service = new fs::Service(ctxt, chord_facade.get(), metadata_mgr);
      fs_client = new fs::Client(ctxt, chord_facade.get());
      fs_facade = std::make_unique<chord::fs::Facade>(ctxt, fs_client, fs_service);
    }

    //--- chord
    chord::Context ctxt;
    chord::Router* router;
    chord::MockClient* client;
    chord::MockService* service;
    //--- fs
    chord::fs::IMetadataManager* metadata_mgr;
    std::unique_ptr<chord::ChordFacade> chord_facade;


    chord::fs::Service* fs_service;
    chord::fs::Client* fs_client;
    std::unique_ptr<chord::fs::Facade> fs_facade;

    //ControlRequest req;
    //ControlResponse res;
};

// FIXME move to chord.fs.service
TEST_F(FsServiceTest, put) {
  PutRequest req;
  PutResponse res;
  ServerContext serverContext;
  //fs_service->put(&serverContext, );
  //Context context = make_context(5);
  //Router router(context);
  //std::unique_ptr<MockStub> stub(new MockStub);

  //MockClient client;
  //chord::Service service(context, &router, &client);

  //const auto status = service.take(nullptr, nullptr, nullptr);

  ////--- assert take has not been called
  //ASSERT_FALSE(status.ok());
}

/*
TEST(ServiceTest, take) {
  Context context = make_context(5);
  Router router(context);

  std::unique_ptr<MockStub> stub(new MockStub);

  MockClient client;
  chord::Service service(context, &router, &client);
  take_producer_t take_producer_cbk = [&](const auto& from, const auto& to) {
    //ASSERT_ cannot be used in non-void returning functions
    EXPECT_EQ(from, uuid_t{"0"});
    EXPECT_EQ(to, uuid_t{"5"});
    return std::vector<chord::TakeResponse>{};
  };
  service.set_take_callback(take_producer_cbk);

  ServerContext serverContext;
  TakeRequest req;
  TakeResponse res;

  req.set_from("0");
  req.set_to("5");

  service.take(&serverContext, &req, nullptr);
}
*/
