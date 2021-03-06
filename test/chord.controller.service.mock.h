#include "chord_controller.grpc.pb.h"

namespace chord {
class MockControllerStub : public chord::controller::Control::StubInterface {
 public:
  MockControllerStub() {}

  MockControllerStub([[maybe_unused]] const MockControllerStub &stub) {
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

  MOCK_METHOD3(PrepareAsynccontrolRaw, grpc::ClientAsyncResponseReaderInterface<chord::controller::ControlResponse>*(
      grpc::ClientContext*, 
      const chord::controller::ControlRequest&,
      grpc::CompletionQueue*));
};
} // namespace chord
