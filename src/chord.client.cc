#include <memory>
#include <grpc/grpc.h>

#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>

#include <boost/log/trivial.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "chord.context.h"
#include "chord.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using chord::Header;

using chord::JoinResponse;
using chord::JoinRequest;
using chord::SuccessorResponse;
using chord::SuccessorRequest;
using chord::StabilizeResponse;
using chord::StabilizeRequest;
using chord::NotifyResponse;
using chord::NotifyRequest;
using chord::Chord;

typedef boost::uuids::uuid uuid_t;

class ChordClient {
private:
  Header make_header(const std::string& src, const std::string& dst) {
    Header header;
    header.set_src(src);
    header.set_dst(dst);
    return header;
  }

  std::shared_ptr<Context> context { nullptr };

public:
  ChordClient(std::shared_ptr<Context> context) :
    context{context}
  {
    BOOST_LOG_TRIVIAL(trace) << "client has uuid " << uuid;
  }

  void join(const std::string& addr) {
    BOOST_LOG_TRIVIAL(debug) << "Joining " << addr;

    ClientContext context;
    JoinRequest req;

    //Header *header = req.mutable_header();
    //header->set_dst("fo");
    //header->set_src("bar");

    JoinResponse res;
    grpc::Status status = stub->join(&context, req, &res);
  }

  void successor(ClientContext* context, const SuccessorRequest* req, SuccessorResponse* res) {
    BOOST_LOG_TRIVIAL(debug) << "received find successor request " << req;
  }
//  
//  void stabilize(ServerContext* context, const StabilizeRequest* req, StabilizeResponse* res) {
//    BOOST_LOG_TRIVIAL(debug) << "received stabilize request " << req;
//    return Status::OK;
//  }
//
//  void notify(ServerContext* context, const NotifyRequest* req, NotifyResponse* res) {
//    BOOST_LOG_TRIVIAL(debug) << "received notification " << req;
//    return Status::OK;
//  }


  void connect(const std::string& addr) {
    BOOST_LOG_TRIVIAL(debug) << "Connecting to " << addr;

    stub = Chord::NewStub(grpc::CreateChannel(addr, grpc::InsecureChannelCredentials()));
    uuid = boost::uuids::random_generator()();

    BOOST_LOG_TRIVIAL(trace) << "Client has uuid " << uuid;

    join(addr);
  }
  
private:
  std::unique_ptr<Chord::Stub> stub;
  uuid_t uuid;
};


//int main() {
//  connect("0.0.0.0:50050");
//  return 0;
//}
