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

using chord::JoinResponse;
using chord::JoinRequest;
using chord::SuccessorResponse;
using chord::SuccessorRequest;
using chord::StabilizeResponse;
using chord::StabilizeRequest;
using chord::NotifyResponse;
using chord::NotifyRequest;
using chord::PutResponse;
using chord::PutRequest;
using chord::GetResponse;
using chord::GetRequest;
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

Status ChordControlService::control(ServerContext* serverContext, const ControlRequest* req, ControlResponse* res) {
  CONTROL_LOG(trace, control) << "received command " << req->command();
  parse_command(req->command());
  return Status::OK;
}

void ChordControlService::parse_command(const string& command) {

  boost::char_separator<char> separator(" ");
  boost::tokenizer< boost::char_separator<char> > tokens(command, separator);
  
  vector<string> words(distance(begin(tokens), end(tokens)));
  copy(begin(tokens), end(tokens), begin(words));

  if(words.size() <= 0) return;
  
  if(words[0] == "put") {
    if(words.size() < 3) return;

    fstream file;
    file.open("test.txt", std::fstream::in | std::fstream::app | std::fstream::binary);

    auto uri = "chord://test.txt";
    peer->put(uri, file);
  }

}

