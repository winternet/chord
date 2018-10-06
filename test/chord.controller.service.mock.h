#include "chord_controller.grpc.pb.h"

class MockControllerStub : public chord::controller::Control::StubInterface {
 public:
  MockControllerStub() {}

  MockControllerStub(const MockControllerStub &stub) {
    (void)stub;
  }

  MOCK_METHOD3(control, grpc::Status(
      grpc::ClientContext*context,
      const chord::controller::ControlRequest&,
      chord::controller::ControlResponse*));

  MOCK_METHOD3(Asynccontrol, grpc::ClientAsyncResponseReaderInterface<chord::controller::ControlResponse>*(
      grpc::ClientContext*,
      const chord::controller::ControlRequest&,
      grpc::CompletionQueue*));

  MOCK_METHOD3(AsynccontrolRaw, grpc::ClientAsyncResponseReaderInterface<chord::controller::ControlResponse>*(
      grpc::ClientContext*,
      const chord::controller::ControlRequest&,
      grpc::CompletionQueue*));

};
