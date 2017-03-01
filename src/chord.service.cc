#include <grpc/grpc.h>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include <boost/log/trivial.hpp>

#include "chord.context.h"
#include "chord.grpc.pb.h"

using grpc::ServerContext;
using grpc::ServerBuilder;
using grpc::Status;

using chord::JoinResponse;
using chord::JoinRequest;
using chord::SuccessorResponse;
using chord::SuccessorRequest;
using chord::StabilizeResponse;
using chord::StabilizeRequest;
using chord::NotifyResponse;
using chord::NotifyRequest;
using chord::Chord;

class ChordServiceImpl final : public Chord::Service {

public:
  ChordServiceImpl(std::shared_ptr<Context> context) :
    context{ context }
  {}

  Status join(ServerContext* context, const JoinRequest* req, JoinResponse* res) {
    BOOST_LOG_TRIVIAL(debug) << "Received join request";

    return Status::OK;
  }

  Status successor(ServerContext* context, const SuccessorRequest* req, SuccessorResponse* res) {
    BOOST_LOG_TRIVIAL(debug) << "received find successor request";

    //if( context.uuid > 
    return Status::OK;
  }

  Status stabilize(ServerContext* context, const StabilizeRequest* req, StabilizeResponse* res) {
    BOOST_LOG_TRIVIAL(debug) << "received stabilize request";
    return Status::OK;
  }

  Status notify(ServerContext* context, const NotifyRequest* req, NotifyResponse* res) {
    BOOST_LOG_TRIVIAL(debug) << "received notification";
    return Status::OK;
  }

  //void start_server(const std::string& addr) {
  //  ChordServiceImpl serviceImpl;

  //  ServerBuilder builder;
  //  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  //  builder.RegisterService(&serviceImpl);

  //  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  //  BOOST_LOG_TRIVIAL(debug) << "server listening on " << addr;
  //  server->Wait();
  //}

private:
  std::shared_ptr<Context> context { nullptr };
};


//int main() {
//  start_server("0.0.0.0:50050");
//  return 0;
//}
