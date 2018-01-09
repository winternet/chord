#include <iostream>
#include <fstream>
#include <experimental/filesystem>

#include <grpc++/server_context.h>

#include <boost/tokenizer.hpp>

#include "chord.log.h"
#include "chord.uri.h"
#include "chord.context.h"
#include "chord_controller.grpc.pb.h"

#include "chord.fs.facade.h"
#include "chord.controller.service.h"

#define log(level) LOG(level) << "[control] "

#define CONTROL_LOG(level,method) LOG(level) << "[control][" << #method << "] "

using grpc::ServerContext;
using grpc::ServerBuilder;
using grpc::Status;

using chord::controller::ControlResponse;
using chord::controller::ControlRequest;

using grpc::ServerBuilder;
using namespace std;

namespace chord {
  namespace controller {
    Service::Service(chord::fs::Facade* filesystem)
      : filesystem { filesystem }
    {
    }

    Status Service::control(ServerContext* serverContext __attribute__((unused)) , const ControlRequest* req, ControlResponse* res) {
      CONTROL_LOG(trace, control) << "received command " << req->command();
      return parse_command(req, res);
    }


    Status Service::parse_command(const ControlRequest* req, ControlResponse* res) {

      const string &command = req->command();
      boost::char_separator<char> separator(" ");
      boost::tokenizer< boost::char_separator<char> > tokenizer(command, separator);

      vector<string> token(distance(begin(tokenizer), end(tokenizer)));
      copy(begin(tokenizer), end(tokenizer), begin(token));

      CONTROL_LOG(trace, parse_command) << "received following token: ";
      for (auto t : token) CONTROL_LOG(trace, parse_command) << "token: " << t;

      if(token.empty()) {
        res->set_result("no commands received.");
        return Status::CANCELLED;
      }

      if(token.at(0) == "put") {
        handle_put(token, res);
      } else if(token.at(0) == "get") {
        handle_get(token, res);
      }

      res->set_result("unknown error.");
      return Status::CANCELLED;
    }

    Status Service::handle_put(const vector<string>& token, ControlResponse* res) {
      if (token.size() < 3) {
        res->set_result("invalid arguments.");
        return Status::CANCELLED;
      }

      auto source = token.at(1);
      auto target = token.at(2);

      try {
        ifstream file;
        file.exceptions(ifstream::failbit | ifstream::badbit);
        file.open(source, std::fstream::binary);

        filesystem->put(uri::from(target), file);
      } catch (const chord::exception& exception) {
        CONTROL_LOG(error, put)
            << "failed to issue put request: " << exception.what();
        return Status::CANCELLED;
      } catch (const std::ios_base::failure& exception) {
        CONTROL_LOG(error, put)
            << "failed to issue put request: " << exception.what();
        return Status::CANCELLED;
      }
      return Status::OK;
    }

    Status Service::handle_get(const vector<string>& token,
                               ControlResponse* res) {
      if (token.size() < 2) {
        res->set_result("invalid arguments.");
        return Status::CANCELLED;
      }

      auto target = token.at(1);

      // if(!fs::is_directory(uri.directory())) {
      //  fs::create_directories(uri.directory());
      //}

      try {
        auto uri = uri::from(target);

        ofstream file;
        file.exceptions(ofstream::failbit | ofstream::badbit);
        file.open(uri.path().filename(), std::fstream::binary);

        filesystem->get(uri, file);
        file.close();
      } catch (const chord::exception& exception) {
        CONTROL_LOG(error, get)
            << "failed to issue put request: " << exception.what();
        return Status::CANCELLED;
      } catch (const std::ios_base::failure& exception) {
        CONTROL_LOG(error, put)
            << "failed to issue put request: " << exception.what();
        return Status::CANCELLED;
      }
      return Status::OK;
    }
  } //namespace controller
} //namespace chord
