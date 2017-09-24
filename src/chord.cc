#include <thread>
#include <chrono>
#include <iostream>

#include <boost/program_options.hpp>

#include "chord.log.h"
#include "chord.peer.h"
#include "chord.control.client.h"
#include "chord.control.service.h"
#include "chord.client.h"
#include "chord.service.h"
#include "chord.context.h"

using namespace std;

namespace po = boost::program_options;

#define fatal cerr << "\nFATAL - "

void parse_program_options(int ac, char* av[], const shared_ptr<Context> context) {
  po::options_description global("[program options]");

  global.add_options()
    ("help,h", "produce help message")
    ("join,j", po::value<endpoint_t>(&(context->join_addr)), "join to an existing address.")
    ("bootstrap,b", "bootstrap peer to create a new chord ring.")
    ("no-controller,n", "do not start the controller.")
    ("uuid,u,id", po::value<uuid_t>(&(context->uuid())), "client uuid.")
    ("bind", po::value<endpoint_t>(&(context->bind_addr)), "bind address that is promoted to clients.")
    ;

  po::variables_map vm;
  po::parsed_options parsed = po::command_line_parser(ac, av)
    .options(global)
    .allow_unregistered()
    .run();
  po::store(parsed, vm);
  po::notify(vm);

  //--- help
  if(vm.count("help")) {
    cout << global << endl;
    exit(1);
  }

  vector<string> commands = po::collect_unrecognized(parsed.options, po::include_positional);
  if(!commands.empty()) {
    ChordControlClient client(context);

    if((commands[0]) == "put") {
      stringstream ss;
      for(auto c:commands) ss << c << " ";
      client.control(ss.str());
    }

    exit(0);
  }

  //--- validate
  if( (!vm.count("join") && !vm.count("bootstrap")) ||
      (vm.count("join") && vm.count("bootstrap")) ) {
    fatal << "please specify either a join address or the bootstrap flag\n\n"
      << global << endl;
    exit(2);
  }

  //--- join
  if(vm.count("join")) {
    LOG(trace) << "[option-join] " << vm["join"].as<string>();
  }

  //--- bootstrap
  if(vm.count("bootstrap")) {
    LOG(trace) << "[option-bootstrap] " << "true";
    context->bootstrap = true;
  }

  //--- interactive
  if(vm.count("no-controller")) {
    LOG(trace) << "[option-no-controller] " << "true";
    context->no_controller = true;
  }

  //--- client-id
  if(vm.count("uuid")) {
    LOG(trace) << "[option-uuid] " << vm["uuid"].as<uuid_t>() << endl;
  }
}

void start_controller(const shared_ptr<ChordPeer>& peer) {
  ChordControlService controller(peer);
}

int main(int argc, char* argv[]) {

  //--- parse program options to context
  //--- or issue client command
  auto context = make_shared<Context>();
  parse_program_options(argc, argv, context);

  //--- start peer
  auto peer = make_shared<ChordPeer>(context);

  //--- start controller
  thread controller_thread(start_controller, peer);

  peer->start();

  //--- join
  controller_thread.join();
  return 0;
}
