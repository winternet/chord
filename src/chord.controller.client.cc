#include <memory>

#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

#include "chord.log.h"
#include "chord.grpc.pb.h"
#include "chord.exception.h"
#include "chord.controller.client.h"

#define log(level) LOG(level) << "[client] "
#define CLIENT_LOG(level, method) LOG(level) << "[client][" << #method << "] "

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using chord::common::Header;
using chord::common::RouterEntry;

using chord::controller::Control;
using chord::controller::ControlResponse;
using chord::controller::ControlRequest;

using namespace std;

namespace chord {
namespace controller {
Client::Client() {
  //--- default stub factory
  make_stub = [&](const endpoint_t &endpoint) {
    return chord::controller::Control::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  };
}

Client::Client(ControlStubFactory make_stub)
    : make_stub{make_stub} {
}

void Client::control(const string &command) {
  ClientContext clientContext;
  ControlRequest req;
  ControlResponse res;

  req.set_command(command);
  //TODO make configurable (at least port)
  auto status = make_stub("127.0.0.1:50000")->control(&clientContext, req, &res);
  if (status.ok()) {
    return;
  }

  throw chord::exception("failed to issue command: " + command);
}
} //namespace controller
} //namespace chord
