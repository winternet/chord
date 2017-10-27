#include <iostream>
#include <fstream>
#include <grpc/grpc.h>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include <boost/tokenizer.hpp>


#include "chord.log.h"
#include "chord.context.h"
#include "chord.grpc.pb.h"

#include "chord.control.service.h"

#define log(level) LOG(level) << "[control] "

#define CONTROL_LOG(level,method) LOG(level) << "[control][" << #method << "] "

using grpc::ServerContext;
using grpc::ServerBuilder;
using grpc::Status;

using chord::ControlResponse;
using chord::ControlRequest;
using chord::Chord;

//#define CONTROL_LOG(level) LOG(level) << "[control] "

using grpc::ServerBuilder;
using namespace std;

ChordControlService::ChordControlService(const shared_ptr<ChordPeer>& peer)
  : peer{ peer }
{
  auto bind_addr = "127.0.0.1:50000"s;
  //endpoint_t bind_addr = context->bind_addr;
  ServerBuilder builder;
  builder.AddListeningPort(bind_addr, grpc::InsecureServerCredentials());
  builder.RegisterService(this);

  unique_ptr<grpc::Server> server(builder.BuildAndStart());
  CONTROL_LOG(debug, init) << "server listening on " << bind_addr;
  //--- blocks
  server->Wait();
}

Status ChordControlService::control(ServerContext* serverContext __attribute__((unused)) , const ControlRequest* req, ControlResponse* res) {
  CONTROL_LOG(trace, control) << "received command " << req->command();
  return parse_command(req, res);
}

Status ChordControlService::parse_command(const ControlRequest* req, ControlResponse* res) {

  const string command = req->command();
  boost::char_separator<char> separator(" ");
  boost::tokenizer< boost::char_separator<char> > tokens(command, separator);
  
  vector<string> words(distance(begin(tokens), end(tokens)));
  copy(begin(tokens), end(tokens), begin(words));

  if(words.size() <= 0) {
    res->set_result("no commands received.");
    return Status::CANCELLED;
  }
  
  if(words.at(0) == "put") {
    if(words.size() < 3) {
      res->set_result("invalid arguments.");
      return Status::CANCELLED;
    }

    auto source = words.at(1);
    auto target = words.at(2);

    ifstream file;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    file.open(source, std::fstream::binary);

    peer->put(target, file);
    return Status::OK;
  }

  res->set_result("unknown error.");
  return Status::CANCELLED;

}

