#include "chord.controller.service.h"

#include <grpcpp/server_context.h>
#include <grpcpp/impl/codegen/status_code_enum.h>

#include <algorithm>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>

#include <boost/tokenizer.hpp>
#include <boost/program_options.hpp>

#include "chord_controller.pb.h"
#include "chord.context.h"
#include "chord.exception.h"
#include "chord.i.fs.facade.h"
#include "chord.log.factory.h"
#include "chord.log.h"
#include "chord.uri.h"

using grpc::ServerContext;
using grpc::Status;

using chord::controller::ControlRequest;
using chord::controller::ControlResponse;

using namespace std;

namespace po = boost::program_options;

namespace chord {
namespace controller {
Service::Service(Context& context, chord::fs::IFacade* filesystem)
    : context{context}
    , filesystem{filesystem}
    , logger{context.logging.factory().get_or_create(logger_name)} {}

Status Service::control(ServerContext* serverContext __attribute__((unused)),
                        const ControlRequest* req, ControlResponse* res) {
  logger->trace("received command {}", req->command());
  return parse_command(req, res);
}

Status Service::parse_command(const ControlRequest* req, ControlResponse* res) {
  const string& command = req->command();
  boost::char_separator<char> separator{" "};
  boost::tokenizer<boost::char_separator<char> > tokenizer{command, separator};

  vector<string> token{static_cast<size_t>(std::distance(begin(tokenizer), end(tokenizer)))};
  copy(begin(tokenizer), end(tokenizer), begin(token));

  logger->trace("received following token");
  for (auto t : token) logger->trace("token: {}", t);

  if (token.empty()) {
    res->set_result("no commands received.");
    return Status(grpc::StatusCode::INVALID_ARGUMENT, "no commands received");
  }

  const string& cmd = token.at(0);
  if (cmd == "put") {
    return handle_put(token, res);
  } else if(cmd == "mkdir") {
    return handle_mkdir(token, res);
  } else if (cmd == "get") {
    return handle_get(token, res);
  } else if (cmd == "dir" || cmd == "ls" || cmd == "ll") {
    return handle_dir(token, res);
  } else if (cmd == "del" || cmd == "rm") {
    return handle_del(token, res);
  }

  res->set_result("unknown error.");
  return Status(grpc::StatusCode::INVALID_ARGUMENT, "failed to handle command", cmd);
}

Status Service::handle_del(const vector<string>& token, ControlResponse* res) {
  // BEG: parse (~>method)
  po::variables_map vm;
  po::positional_options_description pos;
  pos.add("input", -1);
  po::options_description flags("[del flags]");
  flags.add_options()
      ("recursive,r", "remove directories and their contents recursively")
      ("input", po::value<vector<string> >()->default_value({}, ""), "input to process");
  po::parsed_options parsed_flags = po::command_line_parser(token)
    .options(flags)
    .positional(pos)
    .allow_unregistered()
    .run();
  po::store(parsed_flags, vm);
  po::notify(vm);

  auto tokens = vm["input"].as<std::vector<std::string> >();
  const bool recursive = vm.count("recursive");
  if (tokens.size() < 2) {
    res->set_result("invalid arguments.");
    return Status::CANCELLED;
  }
  // END: parse
  

  tokens.erase(tokens.begin());
  for (const auto& target:tokens) {
    const auto status = filesystem->del(uri::from(target), recursive);
    //TODO implement force flag
    if(!status.ok()) {
      logger->error("failed to delete {} (recursive:{}): {} {}", target, recursive, status.error_message(), status.error_details());
      return status;
    }
  }
  return Status::OK;
}

Status Service::handle_mkdir(const vector<string>& token, ControlResponse* res) {
  //TODO: merge with handle_put
  //TODO: handle with parents (-p) ?
  // BEG: parse (~>method)
  po::variables_map vm;
  po::positional_options_description pos;
  pos.add("input", -1);
  po::options_description flags("[mkdir flags]");
  flags.add_options()
      ("repl",  po::value<std::uint32_t>()->default_value(context.replication_cnt), "replication count.")
      ("input", po::value<vector<string> >()->default_value({}, ""), "input to process");
  po::parsed_options parsed_flags = po::command_line_parser(token)
    .options(flags)
    .positional(pos)
    .allow_unregistered()
    .run();
  po::store(parsed_flags, vm);
  po::notify(vm);

  const auto tokens = vm["input"].as<std::vector<std::string> >();
  const auto repl = vm["repl"].as<std::uint32_t>();
  if(tokens.size() < 2) {
    res->set_result("invalid arguments.");
    return Status(grpc::StatusCode::INVALID_ARGUMENT, "failed to handle mkdir", "expecting >= 2 arguments");
  }
  // END: parse
  
  const auto target_it = prev(tokens.end());
  const uri target(*target_it);

  return filesystem->mkdir(target, fs::Replication{repl});
}

Status Service::handle_put(const vector<string>& token, ControlResponse* res) {

  // BEG: parse (~>method)
  po::variables_map vm;
  po::positional_options_description pos;
  pos.add("input", -1);
  po::options_description flags("[put flags]");
  flags.add_options()
      ("repl",  po::value<std::uint32_t>()->default_value(context.replication_cnt), "replication count.")
      ("input", po::value<vector<string> >()->default_value({}, ""), "input to process");
  po::parsed_options parsed_flags = po::command_line_parser(token)
    .options(flags)
    .positional(pos)
    .allow_unregistered()
    .run();
  po::store(parsed_flags, vm);
  po::notify(vm);

  const auto tokens = vm["input"].as<std::vector<std::string> >();
  const auto repl = vm["repl"].as<std::uint32_t>();
  if(tokens.size() < 3) {
    res->set_result("invalid arguments.");
    return Status(grpc::StatusCode::INVALID_ARGUMENT, "failed to handle put", "expecting >= 3 arguments");
  }
  // END: parse

  const auto target_it = prev(tokens.end());
  for (auto it = next(tokens.begin()); it != target_it; ++it) {
    const path source{*it};
    const uri target(*target_it);
    // TODO if taget is no directory rename the file
    //      and put it under that name
    const auto status = filesystem->put(source, target, fs::Replication{repl});
    if(!status.ok()) return status;
  }

  return Status::OK;
}

Status Service::handle_get(const vector<string>& token, ControlResponse* res) {
  //TODO support multiple sources
  if (token.size() < 3) {
    res->set_result("invalid arguments.");
    return Status(grpc::StatusCode::INVALID_ARGUMENT, "failed to handle get", "expecting >= 3 arguments");
  }

  const auto target_it = prev(token.end());
  for (auto it = next(token.begin()); it != target_it; ++it) {
    const uri source{*it};
    const path target{*target_it};
    // TODO if taget is no directory rename the file
    //      and put it under that name
    const auto status = filesystem->get(source, target);
    if(!status.ok())  return status;
  }

  return Status::OK;
}

Status Service::handle_dir(const vector<string>& token, ControlResponse* res) {
  if (token.size() != 2) {
    res->set_result("invalid arguments.");
    return Status(grpc::StatusCode::INVALID_ARGUMENT, "failed to handle dir", "expecting == 2 arguments");
  }

  const auto directory = token.at(1);

  stringstream sstream;
  const auto status = filesystem->dir(uri::from(directory), sstream);
  res->set_result(sstream.str());
  return status;
}
}  // namespace controller
}  // namespace chord
