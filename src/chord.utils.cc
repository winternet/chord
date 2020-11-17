#include <array>

#include "chord.utils.h"
#include "chord.context.h"
#include "chord.context.manager.h"

#include <grpcpp/impl/codegen/status.h>
#include <boost/program_options.hpp>

namespace chord {
namespace utils {

std::string to_string(const grpc::Status& status) {
  return status.error_message() + " ("+status.error_details()+")";
}

uri as_uri(const char* p) {
  return as_uri(path{p});
}

uri as_uri(const path& p) {
  return {"chord", p};
}

Options parse_program_options(int ac, char *av[]) {
  using std::string;
  namespace po = boost::program_options;

  auto logger = chord::log::get_or_create("chord.cc");
  spdlog::set_level(spdlog::level::trace);
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] [%n] %v");

  po::options_description global("[program options]");

  Options options;

  global.add_options()
    ("help,h", "produce help message")
    ("config,c", po::value<string>(), "path to the yaml configuration file.")
    ("join,j", po::value<endpoint>(&(options.context.join_addr)), "join to an existing address.")
    ("bootstrap,b", "bootstrap peer to create a new chord ring.")
    ("no-controller,n", "do not start the controller.")
    ("uuid,u,id", po::value<uuid>(), "client uuid.")
    ("advertise", po::value<endpoint>(&(options.context.advertise_addr)), "advertise address that is publicly promoted on the ring.")
    ("bind", po::value<endpoint>(&(options.context.bind_addr)), "the bind address.")
    ("address,a", po::value<endpoint>(&options.address)->default_value(options.context.bind_addr), "address to control.");

  po::variables_map vm;
  po::parsed_options parsed = po::command_line_parser(ac, av)
                                  .options(global)
                                  .allow_unregistered()
                                  .run();
  po::store(parsed, vm);
  po::notify(vm);

  //--- help
  if (vm.count("help")) {
    std::cout << global << std::endl;
    exit(0);
  }

  bool has_config = vm.count("config");
  //--- read configuration file
  if (has_config) {
    auto config = path{vm["config"].as<string>()};
    options.context = ContextManager::load(config);
  }
  //---

  options.commands = po::collect_unrecognized(parsed.options, po::include_positional);
  if (!options.commands.empty()) {
    return options;
  }

  //--- validate
  if ((!has_config && (!vm.count("join") && !vm.count("bootstrap"))) ||
      (vm.count("join") && vm.count("bootstrap"))) {
    logger->error("please specify either a join address or the bootstrap flag");
    std::cerr << global;
    exit(2);
  }

  //--- uuid
  if (vm.count("uuid")) {
    auto id = vm["uuid"].as<uuid>();
    options.context.set_uuid(id);
    logger->trace("[uuid] {}", id);
  }

  //--- join
  if (vm.count("join")) {
    logger->trace("[option-join] {}", vm["join"].as<string>());
  }

  //--- bootstrap
  if (vm.count("bootstrap")) {
    logger->trace("[option-bootstrap] {}", "true");
    options.context.bootstrap = true;
  }

  //--- interactive
  if (vm.count("no-controller")) {
    logger->trace("[option-no-controller] {}", "true");
    options.context.no_controller = true;
  }

  //--- client-id
  if (vm.count("uuid")) {
    logger->trace("[option-uuid] {}", vm["uuid"].as<uuid>());
  }

  return options;
}
}  // namespace utils
}  // namespace chord
