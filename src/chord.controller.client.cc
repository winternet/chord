#include <memory>

#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

#include "chord.utils.h"
#include "chord.log.h"
#include "chord.grpc.pb.h"
#include "chord.exception.h"
#include "chord.controller.client.h"

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
Client::Client() : make_stub{
  //--- default stub factory
   [&](const endpoint_t &endpoint) {
    return chord::controller::Control::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  }
}{}

Client::Client(ControlStubFactory make_stub)
    : make_stub{make_stub} {
}

void Client::control(const string &command) {
  ClientContext clientContext;
  ControlRequest req;
  ControlResponse res;

  auto logger = chord::log::get_or_create(Client::logger_name);
  logger->debug("issuing command: {}", command);

  req.set_command(command);
  //TODO make configurable (at least port)
  auto status = make_stub("127.0.0.1:50050")->control(&clientContext, req, &res);

  if (!status.ok()) {
  logger->debug("received error: {} {}", status.error_message(), status.error_details());
    throw__grpc_exception(status);
  }
  cout << res.result() << endl;

}
} //namespace controller
} //namespace chord
