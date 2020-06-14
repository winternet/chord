#include "chord.controller.client.h"

#include <iostream>
#include <memory>

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/impl/codegen/client_context.h>
#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/security/credentials.h>

#include "chord_controller.grpc.pb.h"
#include "chord_controller.pb.h"
#include "chord.exception.h"
#include "chord.log.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using chord::controller::Control;
using chord::controller::ControlResponse;
using chord::controller::ControlRequest;

using namespace std;

namespace chord {
namespace controller {
Client::Client() : make_stub{
  //--- default stub factory
   [&](const endpoint& endpoint) {
    return chord::controller::Control::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  }
}{}

Client::Client(ControlStubFactory make_stub)
    : make_stub{make_stub} {
}

void Client::control(const endpoint& address, const string &command) {
  ClientContext clientContext;
  ControlRequest req;
  ControlResponse res;

  auto logger = chord::log::get_or_create(Client::logger_name);
  logger->debug("issuing command: {}", command);

  req.set_command(command);
  //TODO make configurable (at least port)
  auto status = make_stub(address)->control(&clientContext, req, &res);

  if (!status.ok()) {
    logger->debug("received error: {} {}", status.error_message(), status.error_details());
    throw__grpc_exception(status);
  }

  logger->debug("succeeded with command: {}", command);
  cout << res.result() << endl;

}
} //namespace controller
} //namespace chord
