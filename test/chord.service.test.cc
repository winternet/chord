#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "chord.service.h"

using chord::common::Header;
using chord::common::RouterEntry;

using grpc::ServerContext;
using grpc::Status;

using chord::JoinRequest;
using chord::JoinResponse;
using chord::SuccessorRequest;
using chord::SuccessorResponse;

Context make_context(const uuid_t& self) {
  Context context = Context();
  context.set_uuid(self);
  //std::shared_ptr<Context> context = std::shared_ptr<Context>(new Context);
  //context->set_uuid(self);
  
  return context;
}

Router make_router(Context& context) {
  return Router(&context);
}


RouterEntry make_entry(const uuid_t& id, const endpoint_t& addr) {
  RouterEntry entry;
  entry.set_uuid(id);
  entry.set_endpoint(addr);
  return entry;
}

Header make_header(const uuid_t& id, const endpoint_t& addr) {
  Header header;
  header.mutable_src()->CopyFrom(make_entry(id, addr));
  return header;
}

TEST(ServiceTest, join) {
  Context context = Context();
  Router router = Router(&context);
  chord::Service service(context, router);

  //TODO assertions and request

  //ServerContext serverContext;
  //JoinRequest req;
  //JoinResponse res;
  //service.join(&serverContext, &req, &res);
}

/**
 * ring with 1 node
 *   - 0 @ 0.0.0.0:50050
 * node 0 tries to find id 10 -> self
 */
TEST(ServiceTest, successor_single_node) {
  Context context = make_context(0);
  Router router = Router(&context);

  chord::Service service(context, router);

  ServerContext serverContext;
  SuccessorRequest req;
  SuccessorResponse res;

  req.mutable_header()->CopyFrom(make_header(0, "0.0.0.0:50050"));
  req.set_id(uuid_t{"10"});

  service.successor(&serverContext, &req, &res);

  ASSERT_EQ(uuid_t{res.successor().uuid()}, uuid_t{0});
  ASSERT_EQ(res.successor().endpoint(), "0.0.0.0:50050");
}

/**
 * ring with 2 nodes
 *   - 0 @ 0.0.0.0:50050
 *   - 5 @ 0.0.0.0:50055
 * node 0 tries to find successor of id 2 -> 5
 */
TEST(ServiceTest, successor_two_nodes) {
  Context context = make_context(0);
  Router router = make_router(context);

  router.set_successor(0, 5, "0.0.0.0:50055");
  router.set_predecessor(0, 5, "0.0.0.0:50055");

  chord::Service service(context, router);

  ServerContext serverContext;
  SuccessorRequest req;
  SuccessorResponse res;

  req.mutable_header()->CopyFrom(make_header(0, "0.0.0.0:50050"));
  req.set_id("2");

  service.successor(&serverContext, &req, &res);

  ASSERT_EQ(uuid_t(res.successor().uuid()), 5);
  ASSERT_EQ(res.successor().endpoint(), "0.0.0.0:50055");
}

/**
 * ring with 2 nodes
 *   - 0 @ 0.0.0.0:50050
 *   - 5 @ 0.0.0.0:50055
 * node 5 tries to find successor of id 10 -> 0
 */
TEST(ServiceTest, successor_two_nodes_mod) {
  Context context = make_context(5);
  Router router = make_router(context);

  router.set_successor(0, 0, "0.0.0.0:50050");
  router.set_predecessor(0, 0, "0.0.0.0:50050");

  chord::Service service(context, router);

  ServerContext serverContext;
  SuccessorRequest req;
  SuccessorResponse res;

  req.mutable_header()->CopyFrom(make_header(5, "0.0.0.0:50055"));
  req.set_id("10");

  service.successor(&serverContext, &req, &res);

  ASSERT_EQ(res.successor().uuid(), "0");
  ASSERT_EQ(res.successor().endpoint(), "0.0.0.0:50050");
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
	MockStub(const MockStub& stub) {}

  MOCK_METHOD3(successor, grpc::Status(
			grpc::ClientContext* context,
			const chord::SuccessorRequest&,
			chord::SuccessorResponse*));

  MOCK_METHOD3(join, grpc::Status(
			grpc::ClientContext*, 
			const chord::JoinRequest&,
			chord::JoinResponse*));

  MOCK_METHOD3(stabilize, grpc::Status(
			grpc::ClientContext*, 
			const chord::StabilizeRequest&,
			chord::StabilizeResponse*));

	MOCK_METHOD3(notify, grpc::Status(
			grpc::ClientContext*,
			const chord::NotifyRequest&,
			chord::NotifyResponse*));

	MOCK_METHOD3(check, grpc::Status(
			grpc::ClientContext*,
			const chord::CheckRequest&,
			chord::CheckResponse*));

  //MOCK_METHOD2(putRaw, grpc::ClientWriterInterface<chord::PutRequest>*(grpc::ClientContext* context, chord::PutResponse* response));

  //MOCK_METHOD2(getRaw, grpc::ClientReaderInterface<chord::GetResponse>*(grpc::ClientContext* context, const chord::GetRequest& request));

	MOCK_METHOD3(AsyncsuccessorRaw, grpc::ClientAsyncResponseReaderInterface<chord::SuccessorResponse>*(
			grpc::ClientContext*, 
			const chord::SuccessorRequest&,
			grpc::CompletionQueue*));

	MOCK_METHOD3(AsyncjoinRaw, grpc::ClientAsyncResponseReaderInterface<chord::JoinResponse>*(
			grpc::ClientContext*,
			const chord::JoinRequest&,
			grpc::CompletionQueue*));

	MOCK_METHOD3(AsyncstabilizeRaw, grpc::ClientAsyncResponseReaderInterface<chord::StabilizeResponse>*(
			grpc::ClientContext*,
			const chord::StabilizeRequest&,
			grpc::CompletionQueue*));

	MOCK_METHOD3(AsyncnotifyRaw, grpc::ClientAsyncResponseReaderInterface<chord::NotifyResponse>*(
			grpc::ClientContext*,
			const chord::NotifyRequest&,
			grpc::CompletionQueue*));

	MOCK_METHOD3(AsynccheckRaw, grpc::ClientAsyncResponseReaderInterface<chord::CheckResponse>*(
			grpc::ClientContext*,
			const chord::CheckRequest&,
			grpc::CompletionQueue*));

  //MOCK_METHOD4(AsyncputRaw, grpc::ClientAsyncWriterInterface<chord::PutRequest>*(
  //      grpc::ClientContext* context, 
  //      chord::PutResponse* response,
  //      grpc::CompletionQueue* cq,
  //      void* tag));

  //MOCK_METHOD4(AsyncgetRaw, grpc::ClientAsyncReaderInterface<chord::GetResponse>*(
  //      grpc::ClientContext* context,
  //      const ::chord::GetRequest& request,
  //      grpc::CompletionQueue* cq,
  //      void* tag));
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
  Router router = make_router(context);

  router.set_successor(0, 0, "0.0.0.0:50050");
  router.set_predecessor(0, 0, "0.0.0.0:50050");

  std::unique_ptr<MockStub> stub(new MockStub);

	auto stub_factory = [&](const endpoint_t& endpoint){ return std::move(stub); };
  chord::Client client(context, router, stub_factory);

	auto client_factory = [&](){ return client; };
  chord::Service service(context, router, client_factory);

  ServerContext serverContext;
  SuccessorRequest req;
  SuccessorResponse res;

  req.mutable_header()->CopyFrom(make_header(5, "0.0.0.0:50055"));
  req.set_id("2");

  //--- stub's capture parameter
  SuccessorRequest captured_request;
  //--- stub's mocked return parameter
  SuccessorResponse mocked_response;
  RouterEntry* entry = mocked_response.mutable_successor();
  entry->set_uuid(uuid_t{5});
  entry->set_endpoint("0.0.0.0:50055");

  EXPECT_CALL(*stub, successor(_,_,_))
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
