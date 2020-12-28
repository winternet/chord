#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "chord.context.h"
#include "chord.context.manager.h"
#include "chord.controller.client.h"
#include "chord.log.h"
#include "chord.peer.h"
#include "chord.path.h"
#include "chord.types.h"
#include "chord.uuid.h"
#include "chord.utils.h"

using chord::Context;
using chord::ContextManager;
using chord::path;
using chord::endpoint;
using chord::uuid;
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::vector;

int main(int argc, char *argv[]) {
  //--- parse program options to context
  //--- or issue client command
  const auto options = chord::utils::parse_program_options(argc, argv);

  if(!options.commands.empty()) {
    chord::controller::Client controlClient;
    const auto commands = options.commands;

    const string &cmd = commands[0];
    if (cmd == "put" || cmd == "get" || cmd == "dir" || cmd == "ls" || cmd == "ll" ||
        cmd == "del" || cmd == "rm") {
      stringstream ss;
      for (const auto &c : commands) ss << c << " ";
      const auto status = controlClient.control(options.address, ss.str());
      if(!status.ok()) {
        throw__grpc_exception(status);
      }
    }

    exit(0);
  }

  //--- start peer
  auto peer = std::make_unique<chord::Peer>(options.context);

  peer->start();

  return 0;
}
