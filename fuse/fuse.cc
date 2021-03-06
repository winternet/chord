//#include <iostream>
//#include <memory>
//#include <string>
//#include <vector>
//
//#include <boost/program_options.hpp>
//#include "Fuse.h"
//#include "Fuse-impl.h"
//
//// beg: fuse
//#include <fuse.h>
//#include <errno.h>
//#include <string.h>
//// end: fuse
//
//#include "chord.context.h"
//#include "chord.context.manager.h"
//#include "chord.controller.client.h"
//#include "chord.log.h"
//#include "chord.peer.h"
//#include "chord.path.h"
//#include "chord.types.h"
//#include "chord.uuid.h"
//
//using chord::Context;
//using chord::ContextManager;
//using chord::path;
//using chord::endpoint;
//using chord::uuid;
//using std::cerr;
//using std::cout;
//using std::endl;
//using std::string;
//using std::stringstream;
//using std::vector;
//
//namespace po = boost::program_options;
//
//Context parse_program_options(int ac, char *av[]) {
//  auto logger = chord::log::get_or_create("chord.cc");
//  spdlog::set_level(spdlog::level::trace);
//  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] [%n] %v");
//
//  po::options_description global("[program options]");
//
//  Context context;
//  endpoint control_address;
//
//  global.add_options()
//    ("help,h", "produce help message")
//    ("config,c", po::value<string>(), "path to the yaml configuration file.")
//    ("join,j", po::value<endpoint>(&(context.join_addr)), "join to an existing address.")
//    ("bootstrap,b", "bootstrap peer to create a new chord ring.")
//    ("no-controller,n", "do not start the controller.")
//    ("uuid,u,id", po::value<uuid>(), "client uuid.")
//    ("advertise", po::value<endpoint>(&(context.advertise_addr)), "advertise address that is publicly promoted on the ring.")
//    ("bind", po::value<endpoint>(&(context.bind_addr)), "the bind address.")
//    ("address,a", po::value<endpoint>(&control_address)->default_value(context.bind_addr), "address to control.");
//
//  po::variables_map vm;
//  po::parsed_options parsed = po::command_line_parser(ac, av)
//                                  .options(global)
//                                  .allow_unregistered()
//                                  .run();
//  po::store(parsed, vm);
//  po::notify(vm);
//
//  //--- help
//  if (vm.count("help")) {
//    cout << global << endl;
//    exit(1);
//  }
//
//  bool has_config = vm.count("config");
//  //--- read configuration file
//  if (has_config) {
//    auto config = path{vm["config"].as<string>()};
//    context = ContextManager::load(config);
//  }
//  //---
//
//  vector<string> commands =
//      po::collect_unrecognized(parsed.options, po::include_positional);
//  if (!commands.empty()) {
//    chord::controller::Client controlClient;
//
//    const string &cmd = commands[0];
//    if (cmd == "put" || cmd == "get" || cmd == "dir" || cmd == "ls" || cmd == "ll" ||
//        cmd == "del" || cmd == "rm") {
//      stringstream ss;
//      for (const auto &c : commands) ss << c << " ";
//      controlClient.control(control_address, ss.str());
//    }
//
//    exit(0);
//  }
//
//  //--- validate
//  if ((!has_config && (!vm.count("join") && !vm.count("bootstrap"))) ||
//      (vm.count("join") && vm.count("bootstrap"))) {
//    logger->error("please specify either a join address or the bootstrap flag");
//    cerr << global;
//    exit(2);
//  }
//
//  //--- uuid
//  if (vm.count("uuid")) {
//    auto id = vm["uuid"].as<uuid>();
//    context.set_uuid(id);
//    logger->trace("[uuid] {}", id);
//  }
//
//  //--- join
//  if (vm.count("join")) {
//    logger->trace("[option-join] {}", vm["join"].as<string>());
//  }
//
//  //--- bootstrap
//  if (vm.count("bootstrap")) {
//    logger->trace("[option-bootstrap] {}", "true");
//    context.bootstrap = true;
//  }
//
//  //--- interactive
//  if (vm.count("no-controller")) {
//    logger->trace("[option-no-controller] {}", "true");
//    context.no_controller = true;
//  }
//
//  //--- client-id
//  if (vm.count("uuid")) {
//    logger->trace("[option-uuid] {}", vm["uuid"].as<uuid>());
//  }
//  return context;
//}
//
//int main(int argc, char *argv[]) {
//  //--- parse program options to context
//  //--- or issue client command
//  auto context = parse_program_options(argc, argv);
//
//  //--- start peer
//  auto peer = std::make_unique<chord::Peer>(context);
//
//  peer->start();
//
//  return 0;
//}
