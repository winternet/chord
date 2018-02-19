#include <iostream>
#include <thread>

#include <boost/program_options.hpp>

#include "chord.context.h"
#include "chord.context.manager.h"
#include "chord.controller.client.h"
#include "chord.controller.service.h"
#include "chord.file.h"
#include "chord.log.h"
#include "chord.peer.h"

using namespace std;
using namespace chord;

namespace po = boost::program_options;

#define fatal cerr << "\nFATAL - "

Context parse_program_options(int ac, char *av[]) {
  po::options_description global("[program options]");

  Context context;

  global.add_options()("help,h", "produce help message")(
      "config,c", po::value<string>(), "path to the yaml configuration file.")(
      "join,j", po::value<endpoint_t>(&(context.join_addr)),
      "join to an existing address.")(
      "bootstrap,b", "bootstrap peer to create a new chord ring.")(
      "no-controller,n", "do not start the controller.")(
      "uuid,u,id", po::value<uuid_t>(), "client uuid.")(
      "bind", po::value<endpoint_t>(&(context.bind_addr)),
      "bind address that is promoted to clients.");

  po::variables_map vm;
  po::parsed_options parsed = po::command_line_parser(ac, av)
                                  .options(global)
                                  .allow_unregistered()
                                  .run();
  po::store(parsed, vm);
  po::notify(vm);

  //--- help
  if (vm.count("help")) {
    cout << global << endl;
    exit(1);
  }

  bool has_config = vm.count("config");
  //--- read configuration file
  if (has_config) {
    auto config = path{vm["config"].as<string>()};
    context = ContextManager::load(config);
  }
  //---

  vector<string> commands =
      po::collect_unrecognized(parsed.options, po::include_positional);
  if (!commands.empty()) {
    chord::controller::Client controlClient;

    const string &cmd = commands[0];
    if (cmd == "put" || cmd == "get" || cmd == "dir" || cmd == "ls" || cmd == "ll" ||
        cmd == "del" || cmd == "rm") {
      stringstream ss;
      for (const auto &c : commands) ss << c << " ";
      controlClient.control(ss.str());
    }

    exit(0);
  }

  //--- validate
  if (!has_config && (!vm.count("join") && !vm.count("bootstrap")) ||
      (vm.count("join") && vm.count("bootstrap"))) {
    fatal << "please specify either a join address or the bootstrap flag\n\n"
          << global << endl;
    exit(2);
  }

  //--- uuid
  if (vm.count("uuid")) {
    auto id = vm["uuid"].as<uuid_t>();
    context.set_uuid(id);
    LOG(trace) << "[uuid] " << id;
  }

  //--- join
  if (vm.count("join")) {
    LOG(trace) << "[option-join] " << vm["join"].as<string>();
  }

  //--- bootstrap
  if (vm.count("bootstrap")) {
    LOG(trace) << "[option-bootstrap] "
               << "true";
    context.bootstrap = true;
  }

  //--- interactive
  if (vm.count("no-controller")) {
    LOG(trace) << "[option-no-controller] "
               << "true";
    context.no_controller = true;
  }

  //--- client-id
  if (vm.count("uuid")) {
    LOG(trace) << "[option-uuid] " << vm["uuid"].as<uuid_t>() << endl;
  }
  return context;
}

// void start_controller(const shared_ptr<chord::fs::Client>& fs_client) {
//  chord::controller::Service controller(fs_client);
//}

int main(int argc, char *argv[]) {
  //--- parse program options to context
  //--- or issue client command
  auto context = parse_program_options(argc, argv);

  //--- start peer
  auto peer = make_shared<chord::Peer>(context);

  //--- start controller
  // thread controller_thread([&](){
  //    auto fs_client = make_shared<chord::fs::Client>(context, peer);
  //    chord::controller::Service controller(fs_client);
  //});

  peer->start();

  //--- join
  // controller_thread.join();
  return 0;
}
