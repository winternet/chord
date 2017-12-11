#include <iostream>
#include <fstream>
#include <experimental/filesystem>
#include <grpc/grpc.h>

//#include <grpc++/server.h>
//#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
//#include <grpc++/security/server_credentials.h>

#include <boost/tokenizer.hpp>


#include "chord.log.h"
#include "chord.uri.h"
#include "chord.context.h"
#include "chord.exception.h"
#include "chord_controller.grpc.pb.h"

#include "chord.fs.client.h"
#include "chord.controller.service.h"

#define log(level) LOG(level) << "[control] "

#define CONTROL_LOG(level,method) LOG(level) << "[control][" << #method << "] "

using grpc::ServerContext;
using grpc::ServerBuilder;
using grpc::Status;

using chord::controller::ControlResponse;
using chord::controller::ControlRequest;
//using chord::controller::Chord;

//#define CONTROL_LOG(level) LOG(level) << "[control] "

using grpc::ServerBuilder;
using namespace std;

namespace fs = std::experimental::filesystem;

namespace chord {
  namespace controller {
    Service::Service(shared_ptr<chord::fs::Client> fs_client)
      : fs_client { fs_client }
    {
    }

    Status Service::control(ServerContext* serverContext __attribute__((unused)) , const ControlRequest* req, ControlResponse* res) {
      CONTROL_LOG(trace, control) << "received command " << req->command();
      return parse_command(req, res);
    }

    Status Service::parse_command(const ControlRequest* req, ControlResponse* res) {

      const string command = req->command();
      boost::char_separator<char> separator(" ");
      boost::tokenizer< boost::char_separator<char> > tokens(command, separator);

      vector<string> words(distance(begin(tokens), end(tokens)));
      copy(begin(tokens), end(tokens), begin(words));

      CONTROL_LOG(error,parse_command) << "received following words: ";
      for( auto w:words ) CONTROL_LOG(error, parse_command) << "word " << w;

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

        try {
          fs_client->put(target, file);
        } catch(chord::exception exception) {
          CONTROL_LOG(error,put) << "failed to issue put request: " << exception.what();
          return Status::CANCELLED;
        }
        return Status::OK;
      } else if(words.at(0) == "get") {
        if(words.size() < 2) {
          res->set_result("invalid arguments.");
          return Status::CANCELLED;
        }

        auto target = words.at(1);

        auto uri = chord::uri::from(target);
        //if(!fs::is_directory(uri.directory())) {
        //  fs::create_directories(uri.directory());
        //}

        ofstream file;
        file.exceptions(ofstream::failbit | ofstream::badbit);
        file.open(uri.filename(), std::fstream::binary);

        try {
          fs_client->get(target, file);
          file.close();
        } catch(chord::exception exception) {
          CONTROL_LOG(error,get) << "failed to issue put request: " << exception.what();
          return Status::CANCELLED;
        }
        return Status::OK;
      }

      res->set_result("unknown error.");
      return Status::CANCELLED;

    }
  } //namespace controller
} //namespace chord
