#include <experimental/filesystem>
#include <fstream>
#include <iostream>

#include <grpc++/server_context.h>

#include <boost/tokenizer.hpp>

#include "chord.context.h"
#include "chord.file.h"
#include "chord.log.h"
#include "chord.uri.h"
#include "chord_controller.grpc.pb.h"

#include "chord.controller.service.h"
#include "chord.fs.facade.h"


using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using chord::controller::ControlRequest;
using chord::controller::ControlResponse;

using grpc::ServerBuilder;
using namespace std;

namespace chord {
namespace controller {
Service::Service(chord::fs::Facade* filesystem) : filesystem{filesystem} {
  logger = spdlog::stdout_color_mt("chord.controller.service");
}

Status Service::control(ServerContext* serverContext __attribute__((unused)),
                        const ControlRequest* req, ControlResponse* res) {
  logger->trace("received command {}", req->command());
  return parse_command(req, res);
}

Status Service::parse_command(const ControlRequest* req, ControlResponse* res) {
  const string& command = req->command();
  boost::char_separator<char> separator{" "};
  boost::tokenizer<boost::char_separator<char> > tokenizer{command, separator};

  vector<string> token{distance(begin(tokenizer), end(tokenizer))};
  copy(begin(tokenizer), end(tokenizer), begin(token));

  logger->trace("received following token");
  for (auto t : token) logger->trace("token: {}", t);

  if (token.empty()) {
    res->set_result("no commands received.");
    return Status::CANCELLED;
  }

  const string& cmd = token.at(0);
  if (cmd == "put") {
    return handle_put(token, res);
  } else if (cmd == "get") {
    return handle_get(token, res);
  } else if (cmd == "dir" || cmd == "ls" || cmd == "ll") {
    return handle_dir(token, res);
  } else if (cmd == "del" || cmd == "rm") {
    return handle_del(token, res);
  }

  res->set_result("unknown error.");
  return Status::CANCELLED;
}

Status Service::handle_del(const vector<string>& token, ControlResponse* res) {
  if (token.size() != 2) {
    res->set_result("invalid arguments.");
    return Status::CANCELLED;
  }

  auto directory = token.at(1);

  try {
    filesystem->del(uri::from(directory));
  } catch (const chord::exception& exception) {
    logger->error("failed to issue del request: {}", exception.what());
    return Status::CANCELLED;
  }
  return Status::OK;
}


Status Service::handle_put(const vector<string>& token, ControlResponse* res) {
  //TODO support multiple sources
  if (token.size() < 3) {
    res->set_result("invalid arguments.");
    return Status::CANCELLED;
  }

  try {
    const auto target_it = prev(token.end());
    for (auto it = next(token.begin()); it != target_it; ++it) {
      const path& source = {*it};
      const uri& target = {*target_it};
      // TODO if taget is no directory rename the file
      //      and put it under that name
      filesystem->put(source, target);
    }
  } catch (const chord::exception& exception) {
    logger->error("failed to issue put request: {}", exception.what());
    return Status::CANCELLED;
  }
  return Status::OK;
}

Status Service::handle_get(const vector<string>& token, ControlResponse* res) {
  //TODO support multiple sources
  if (token.size() < 2) {
    res->set_result("invalid arguments.");
    return Status::CANCELLED;
  }

  try {
    const auto target_it = prev(token.end());
    for (auto it = next(token.begin()); it != target_it; ++it) {
      const uri& source = {*it};
      const path& target = {*target_it};
      // TODO if taget is no directory rename the file
      //      and put it under that name
      filesystem->get(source, target);
    }
  } catch (const chord::exception& exception) {
    logger->error("failed to issue get request: {}", exception.what());
    return Status::CANCELLED;
  }

  return Status::OK;
}

Status Service::handle_dir(const vector<string>& token, ControlResponse* res) {
  if (token.size() != 2) {
    res->set_result("invalid arguments.");
    return Status::CANCELLED;
  }

  auto directory = token.at(1);

  try {
    stringstream sstream;
    filesystem->dir(uri::from(directory), sstream);
    res->set_result(sstream.str());
  } catch (const chord::exception& exception) {
    logger->error("failed to issue dir request: {}", exception.what());
    return Status::CANCELLED;
  }
  return Status::OK;
}
}  // namespace controller
}  // namespace chord
