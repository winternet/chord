#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <grpcpp/server_context.h>
#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>

#include <grpcpp/impl/codegen/status.h>

#include <memory>
#include <utility>
#include <vector>

#include "chord.common.h"
#include "chord.client.h"
#include "chord.client.mock.h"
#include "chord.context.h"
#include "chord.node.h"
#include "chord.pb.h"
#include "chord.router.h"
#include "chord.service.h"
#include "chord.types.h"
#include "chord.uuid.h"
#include "chord_common.pb.h"
#include "chord.signal.h"
#include "chord.log.h"
#include "chord.router.spy.h"
#include "util/chord.test.helper.h"

using chord::common::make_request;
using chord::common::Header;
using chord::common::RouterEntry;

using grpc::ServerContext;
using grpc::Status;

using chord::JoinRequest;
using chord::JoinResponse;
using chord::SuccessorRequest;
using chord::SuccessorResponse;

using ::testing::Eq;

using namespace chord::test;
using namespace chord;

template<typename T>
void nop(T* val) {}
template<typename T>
std::unique_ptr<T, void(*)(T*)> unique_of(T& val) {
  return std::unique_ptr<T, void(*)(T*) >(&val, nop);
}


TEST(ServiceTest, join) {
  spdlog::set_level(spdlog::level::trace);
  JoinRequest req;
  JoinResponse res;
  ServerContext serverContext;
  Context context = Context();
  context.set_uuid(50);
  RouterSpy router(context);
	MockClient client;
  Service service(context, &router, &client);

  // assert we're the only one
  //ASSERT_EQ(router.size(), 1);

  bool callback_called = false;

  // join from 1
  req.mutable_header()->CopyFrom(make_header({1}, "1.1.1.1:1111"));
  service.on_predecessor_update().connect([&](const chord::node old_predecessor, const chord::node new_predecessor) {
      callback_called = true;
      ASSERT_EQ(old_predecessor.endpoint, "0.0.0.0:50050");
      ASSERT_EQ(old_predecessor.uuid, uuid_t{50});
      ASSERT_EQ(new_predecessor.endpoint, "1.1.1.1:1111");
      ASSERT_EQ(new_predecessor.uuid, uuid_t{1});
  });
  service.join(&serverContext, &req, &res);

  /**
   * NOTE router is updated by the joining node after
   * joining node notifies its new successor/predecessor
   */
  //ASSERT_EQ(router.size(), 1);
  auto successor = router.successor();
  auto predecessor = router.predecessor();
  ASSERT_FALSE(router.has_successor());
  ASSERT_FALSE(router.has_predecessor());

  //ASSERT_FALSE(successor);
  ASSERT_EQ(successor, context.node());
  ASSERT_FALSE(predecessor);

  //--- response
  ASSERT_EQ(res.header().src().uuid(), "50");
  ASSERT_EQ(res.header().src().endpoint(), "0.0.0.0:50050");
  ASSERT_EQ(res.predecessor().uuid(), "50");
  ASSERT_EQ(res.predecessor().endpoint(), "0.0.0.0:50050");
  ASSERT_EQ(res.successor().uuid(), "50");
  ASSERT_EQ(res.successor().endpoint(), "0.0.0.0:50050");
}

/**
 * ring with 1 node
 *   - 0 @ 0.0.0.0:50050
 * node 0 tries to find id 10 -> self
 */
TEST(ServiceTest, successor_single_node) {
  Context context = make_context(0);
  Router router(context);
	MockClient client;
  Service service(context, &router, &client);

  const auto router_entry = service.successor({10});

  ASSERT_EQ(router_entry.uuid(), "0");
  ASSERT_EQ(router_entry.endpoint(), "0.0.0.0:50050");
}

/**
 * ring with 2 nodes
 *   - 0 @ 0.0.0.0:50050
 *   - 5 @ 0.0.0.0:50055
 * node 0 tries to find successor of id 2 -> 5
 */
TEST(ServiceTest, successor_two_nodes) {
  Context context = make_context(0);
  Router router(context);
	MockClient client;

  router.update({5, "0.0.0.0:50055"});
  router.update({5, "0.0.0.0:50055"});
  //router.set_successor(0, {5, "0.0.0.0:50055"});
  //router.set_predecessor(0, {5, "0.0.0.0:50055"});

  Service service(context, &router, &client);

  const auto entry = service.successor({2});

  ASSERT_EQ(entry.uuid(), "5");
  ASSERT_EQ(entry.endpoint(), "0.0.0.0:50055");
}

/**
 * ring with 2 nodes
 *   - 0 @ 0.0.0.0:50050
 *   - 5 @ 0.0.0.0:50055
 * node 5 tries to find successor of id 10 -> 0
 */
TEST(ServiceTest, successor_two_nodes_mod) {
  Context context = make_context(5);
  Router router(context);
	MockClient client;

  router.update({0, "0.0.0.0:50050"});
  router.update({0, "0.0.0.0:50050"});
  //router.set_successor(0, {0, "0.0.0.0:50050"});
  //router.set_predecessor(0, {0, "0.0.0.0:50050"});

  chord::Service service(context, &router, &client);

  const auto entry = service.successor({10});

  ASSERT_EQ(entry.uuid(), "0");
  ASSERT_EQ(entry.endpoint(), "0.0.0.0:50050");
}


//---             ---//
//--- mocked test ---//
//---             ---//

using ::testing::Return;
using ::testing::_;
using ::testing::Invoke;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::SetArgPointee;
using ::testing::DoAll;

class MockStub : public chord::Chord::StubInterface {
 public:
  MockStub() {}

  MockStub(const MockStub &stub) {
    (void)stub;
  }

  MOCK_METHOD3(successor, grpc::Status(
      grpc::ClientContext*context,
      const chord::SuccessorRequest&,
      chord::SuccessorResponse*));

  MOCK_METHOD3(join, grpc::Status(
      grpc::ClientContext*,
      const chord::JoinRequest&,
      chord::JoinResponse*));

  MOCK_METHOD3(leave, grpc::Status(
      grpc::ClientContext*, 
      const ::chord::LeaveRequest&,
      chord::LeaveResponse*));

  MOCK_METHOD3(stabilize, grpc::Status(
      grpc::ClientContext*,
      const chord::StabilizeRequest&,
      chord::StabilizeResponse*));

  MOCK_METHOD3(notify, grpc::Status(
      grpc::ClientContext*,
      const chord::NotifyRequest&,
      chord::NotifyResponse*));

  MOCK_METHOD3(ping, grpc::Status(
      grpc::ClientContext*,
      const chord::PingRequest&,
      chord::PingResponse*));

  MOCK_METHOD3(PrepareAsyncsuccessorRaw, grpc::ClientAsyncResponseReaderInterface<chord::SuccessorResponse>*(
      grpc::ClientContext*,
      const chord::SuccessorRequest&,
      grpc::CompletionQueue*));

  MOCK_METHOD3(AsyncsuccessorRaw, grpc::ClientAsyncResponseReaderInterface<chord::SuccessorResponse>*(
      grpc::ClientContext*,
      const chord::SuccessorRequest&,
      grpc::CompletionQueue*));

  MOCK_METHOD3(PrepareAsyncjoinRaw, grpc::ClientAsyncResponseReaderInterface<chord::JoinResponse>*(
      grpc::ClientContext*,
      const chord::JoinRequest&,
      grpc::CompletionQueue*));

  MOCK_METHOD3(AsyncleaveRaw, grpc::ClientAsyncResponseReaderInterface<chord::LeaveResponse>*(
      grpc::ClientContext*,
      const chord::LeaveRequest&,
      grpc::CompletionQueue*));

  MOCK_METHOD3(PrepareAsyncleaveRaw, grpc::ClientAsyncResponseReaderInterface<chord::LeaveResponse>*(
      grpc::ClientContext*,
      const chord::LeaveRequest&,
      grpc::CompletionQueue*));

  MOCK_METHOD3(AsyncjoinRaw, grpc::ClientAsyncResponseReaderInterface<chord::JoinResponse>*(
      grpc::ClientContext*,
      const chord::JoinRequest&,
      grpc::CompletionQueue*));

  MOCK_METHOD3(PrepareAsyncstabilizeRaw, grpc::ClientAsyncResponseReaderInterface<chord::StabilizeResponse>*(
      grpc::ClientContext*,
      const chord::StabilizeRequest&,
      grpc::CompletionQueue*));

  MOCK_METHOD3(AsyncstabilizeRaw, grpc::ClientAsyncResponseReaderInterface<chord::StabilizeResponse>*(
      grpc::ClientContext*,
      const chord::StabilizeRequest&,
      grpc::CompletionQueue*));

  MOCK_METHOD3(PrepareAsyncnotifyRaw, grpc::ClientAsyncResponseReaderInterface<chord::NotifyResponse>*(
      grpc::ClientContext*,
      const chord::NotifyRequest&,
      grpc::CompletionQueue*));

  MOCK_METHOD3(AsyncnotifyRaw, grpc::ClientAsyncResponseReaderInterface<chord::NotifyResponse>*(
      grpc::ClientContext*,
      const chord::NotifyRequest&,
      grpc::CompletionQueue*));

  MOCK_METHOD3(PrepareAsyncpingRaw, grpc::ClientAsyncResponseReaderInterface<chord::PingResponse>*(
      grpc::ClientContext*,
      const chord::PingRequest&,
      grpc::CompletionQueue*));

  MOCK_METHOD3(AsyncpingRaw, grpc::ClientAsyncResponseReaderInterface<chord::PingResponse>*(
      grpc::ClientContext*,
      const chord::PingRequest&,
      grpc::CompletionQueue*));
};

/**
 * ring with 2 nodes
 *   - 0 @ 0.0.0.0:50050
 *   - 5 @ 0.0.0.0:50055
 * node 5 tries to find successor of id 2 -> 5
 * 
 * we have to mock the client class in order to 
 * assert that node successor(0) gets called.
 */
TEST(ServiceTest, successor_two_nodes_modulo) {
  Context context = make_context(5);
  Router router(context);

  router.update({0, "0.0.0.0:50050"});
  router.update({0, "0.0.0.0:50050"});
  //router.set_successor(0, {0, "0.0.0.0:50050"});
  //router.set_predecessor(0, {0, "0.0.0.0:50050"});

  std::unique_ptr<MockStub> stub(new MockStub);
  auto stub_factory = [&](const endpoint& endpoint) { (void)endpoint; return std::move(stub); };

  Client client{context, &router, stub_factory};
  Service service(context, &router, &client);

  ServerContext serverContext;
  SuccessorRequest req = make_request<SuccessorRequest>(context);
  SuccessorResponse res;

  //req.mutable_header()->CopyFrom(make_header(5, "0.0.0.0:50055"));
  req.set_id("2");

  //--- stub's capture parameter
  SuccessorRequest captured_request;
  //--- stub's mocked return parameter
  SuccessorResponse mocked_response;
  RouterEntry *entry = mocked_response.mutable_successor();
  entry->set_uuid(uuid_t{5});
  entry->set_endpoint("0.0.0.0:50055");

  EXPECT_CALL(*stub, successor(_, _, _))
      .Times(1)
      .WillOnce(DoAll(
          SaveArg<1>(&captured_request),
          SetArgPointee<2>(mocked_response),
          Return(Status::OK)));

  service.successor(&serverContext, &req, &res);

  //--- stub has been called with wanted id 2
  ASSERT_EQ(captured_request.id(), "2");

  //--- service returns the mocked response
  ASSERT_EQ(res.has_successor(), true);
  ASSERT_EQ(res.successor().uuid(), "5");
  ASSERT_EQ(res.successor().endpoint(), "0.0.0.0:50055");

}


TEST(ServiceTest, stabilize__without_predecessor) {
  Context context = make_context(5);
  Router router(context);

  std::unique_ptr<MockStub> stub(new MockStub);

  MockClient client;
  chord::Service service(context, &router, &client);

  ServerContext serverContext;
  StabilizeRequest req;
  StabilizeResponse res;

  const auto status = service.stabilize(&serverContext, &req, &res);

  //--- no predecessor returned from router
  ASSERT_TRUE(status.ok());
  ASSERT_TRUE(res.has_header());
  ASSERT_FALSE(res.has_predecessor());
}

TEST(ServiceTest, stabilize) {
  Context context = make_context(5);
  Router router(context);
  router.update(node{"1", "1.1.1.1:8888"});
  //router.set_predecessor(0, {"1", "1.1.1.1:8888"});

  std::unique_ptr<MockStub> stub(new MockStub);

  MockClient client;
  chord::Service service(context, &router, &client);

  ServerContext serverContext;
  StabilizeRequest req;
  StabilizeResponse res;

  const auto status = service.stabilize(&serverContext, &req, &res);

  //--- no predecessor returned from router
  ASSERT_TRUE(status.ok());
  ASSERT_TRUE(res.has_header());
  ASSERT_TRUE(res.has_predecessor());
  ASSERT_EQ(res.predecessor().uuid(), "1");
  ASSERT_EQ(res.predecessor().endpoint(), "1.1.1.1:8888");
}

TEST(ServiceTest, leave__without_header) {
  Context context = make_context(5);
  Router router(context);

  std::unique_ptr<MockStub> stub(new MockStub);

  MockClient client;
  chord::Service service(context, &router, &client);

  ServerContext serverContext;
  LeaveRequest req;
  LeaveResponse res;

  const auto status = service.leave(&serverContext, &req, &res);

  ASSERT_FALSE(status.ok());
}

//TEST(ServiceTest, leave__without_callback) {
//  Context context = make_context(5);
//  Router router(context);
//
//  std::unique_ptr<MockStub> stub(new MockStub);
//
//  MockClient client;
//  Service service(context, &router, &client);
//
//  ServerContext serverContext;
//  LeaveRequest req;
//  LeaveResponse res;
//
//  const node to_node{"2", "2.2.2.2:8888"};
//  const node from_node{"8", "8.8.8.8:8888"};
//  auto src = req.mutable_header()->mutable_src();
//  src->set_uuid(to_node.uuid.string());
//  src->set_endpoint(to_node.endpoint);
//
//  auto pred = req.mutable_predecessor();
//  pred->set_uuid(from_node.uuid.string());
//  pred->set_endpoint(from_node.endpoint);
//
//  const auto status = service.leave(&serverContext, &req, &res);
//
//  ASSERT_FALSE(status.ok());
//}


// FIXME move to chord.fs.service.test
//TEST(ServiceTest, leave) {
//  Context context = make_context(5);
//  Router router(context);
//
//  MockClient client;
//  chord::Service service{context, &router, &client};
//  const auto callback = [](const TakeResponse& tres) {
//    (void)tres;
//  };
//  service.set_on_leave_callback(callback);
//
//  ServerContext serverContext;
//  LeaveRequest req;
//  LeaveResponse res;
//
//  const node to_node{"2", "2.2.2.2:8888"};
//  const node from_node{"8", "8.8.8.8:8888"};
//  auto src = req.mutable_header()->mutable_src();
//  src->set_uuid(to_node.uuid.string());
//  src->set_endpoint(to_node.endpoint);
//
//  auto pred = req.mutable_predecessor();
//  pred->set_uuid(from_node.uuid.string());
//  pred->set_endpoint(from_node.endpoint);
//
//  EXPECT_CALL(client, take(Eq(uuid_t{"8"}), Eq(uuid_t{"2"}), Eq(to_node), _));
//  const auto status = service.leave(&serverContext, &req, &res);
//
//  ASSERT_TRUE(status.ok());
//}

TEST(ServiceTest, notify__validate) {
  Context context = make_context(5);
  Service service(context, nullptr, nullptr);

  ServerContext serverContext;
  NotifyRequest req;
  NotifyResponse res;

  // header not set -> validation should fail
  const auto status = service.notify(&serverContext, &req, &res);

  ASSERT_FALSE(status.ok());
}

TEST(ServiceTest, notify__initial_predecessor) {
  Context context = make_context(20);
  Router router(context);

  Service service{context, &router, nullptr};

  ServerContext serverContext;
  NotifyRequest req;
  NotifyResponse res;

  const node from_node{"10", "10.10.10.10:8888"};
  auto src = req.mutable_header()->mutable_src();
  src->set_uuid(from_node.uuid.string());
  src->set_endpoint(from_node.endpoint);

  const auto status = service.notify(&serverContext, &req, &res);

  ASSERT_TRUE(status.ok());
  ASSERT_EQ(router.predecessor(), from_node);
}

TEST(ServiceTest, notify__update_predecessor) {
  Context context = make_context(20);

  Router router(context);
  router.update(node{"0", "0.0.0.0:8888"});
  //router.set_predecessor(0, {"0", "0.0.0.0:8888"});

  Service service{context, &router, nullptr};

  ServerContext serverContext;
  NotifyRequest req;
  NotifyResponse res;

  const node from_node{"10", "10.10.10.10:8888"};
  auto src = req.mutable_header()->mutable_src();
  src->set_uuid(from_node.uuid.string());
  src->set_endpoint(from_node.endpoint);

  const auto status = service.notify(&serverContext, &req, &res);

  ASSERT_TRUE(status.ok());
  ASSERT_EQ(router.predecessor(), from_node);
}

TEST(ServiceTest, notify__no_update) {
  Context context = make_context(20);

  Router router(context);

  const node pred{"10", "10.10.10.10:8888"};
  router.update(pred);

  Service service{context, &router, nullptr};

  ServerContext serverContext;
  NotifyRequest req;
  NotifyResponse res;

  const node from_node{"1", "1.1.1.1:8888"};
  auto src = req.mutable_header()->mutable_src();
  src->set_uuid(from_node.uuid.string());
  src->set_endpoint(from_node.endpoint);

  const auto status = service.notify(&serverContext, &req, &res);

  ASSERT_TRUE(status.ok());
  // no update
  ASSERT_EQ(router.predecessor(), pred);
}

TEST(ServiceTest, ping) {
  Context context = make_context(20);

  Router router(context);

  Service service{context, &router, nullptr};

  ServerContext serverContext;
  PingRequest req;
  PingResponse res;

  const node from_node{"1", "1.1.1.1:8888"};
  auto src = req.mutable_header()->mutable_src();
  src->set_uuid(from_node.uuid.string());
  src->set_endpoint(from_node.endpoint);

  const auto status = service.ping(&serverContext, &req, &res);

  ASSERT_TRUE(status.ok());
  ASSERT_TRUE(res.has_header());
  ASSERT_EQ(res.header().src().uuid(), context.uuid().string());
  ASSERT_EQ(res.header().src().endpoint(), context.bind_addr);
}

TEST(ServiceTest, fix_fingers) {
  Context context = make_context(20);

  RouterSpy router(context);

  const node succ{"50", "5.5.5.5:8888"};
  router.update(succ);

  Service service{context, &router, nullptr};

  service.fix_fingers(2);
  ASSERT_EQ(router.entry(1).node(), succ); // no holes in successors
  ASSERT_EQ(router.entry(2).node(), succ);
}
