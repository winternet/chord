#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

#include "chord.types.h"
#include "chord.exception.h"
#include "chord.controller.client.h"
#include "chord.controller.service.mock.h"

using namespace std;
using namespace chord::controller;

using chord::endpoint;

using grpc::Status;

using ::testing::Return;
using ::testing::_;
using ::testing::Invoke;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::SetArgPointee;
using ::testing::DoAll;

using chord::MockControllerStub;

TEST(chord_controller_client, default_constructor) {
  Client client;
}

TEST(chord_controller_client, custom_stub_constructor) {
  const auto make_stub = [&](const endpoint& endpoint) {
    return Control::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  };
  Client client(make_stub);
}

TEST(chord_controller_client, control) {
  std::unique_ptr<MockControllerStub> stub(new MockControllerStub);
  const auto make_stub = [&]([[maybe_unused]] const endpoint& endpoint) {
    return std::move(stub);
  };
  Client client(make_stub);


  ControlRequest captured_request;
  //--- stub's mocked return parameter
  ControlResponse mocked_response;
  mocked_response.set_result("foobar");

  EXPECT_CALL(*stub, control(_, _, _))
      .Times(1)
      .WillOnce(DoAll(
          SaveArg<1>(&captured_request),
          SetArgPointee<2>(mocked_response),
          Return(Status::OK)));

  std::string req{"dir chord:///folder"};
  client.control("localhost:50050", req);

  ASSERT_EQ(req, captured_request.command());
}

TEST(chord_controller_client, control_with_exception) {
  std::unique_ptr<MockControllerStub> stub(new MockControllerStub);
  const auto make_stub = [&]([[maybe_unused]] const endpoint& endpoint) {
    return std::move(stub);
  };
  Client client(make_stub);


  ControlRequest captured_request;
  //--- stub's mocked return parameter
  ControlResponse mocked_response;
  mocked_response.set_result("foobar");

  EXPECT_CALL(*stub, control(_, _, _))
      .Times(1)
      .WillOnce(DoAll(
          SaveArg<1>(&captured_request),
          SetArgPointee<2>(mocked_response),
          Return(Status::CANCELLED)));

  const auto status = client.control("localhost:50050", "dir chord:///folder");
  ASSERT_FALSE(status.ok());
  ASSERT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
  //ASSERT_THROW(client.control("localhost:50050", "dir chord:///folder"), chord::exception);
}
