#include <thread>
#include <chrono>
#include <iostream>

#include <boost/program_options.hpp>

#include "chord.log.h"
#include "chord.peer.cc"
#include "chord.client.h"
#include "chord.service.h"
#include "chord.context.h"

namespace po = boost::program_options;

#define fatal std::cerr << "\nFATAL - "

void parse_program_options(int ac, char* av[], const std::shared_ptr<Context>& context) {
  po::options_description desc("[program options]");

  desc.add_options()
    ("help,h", "produce help message")
    ("join,j", po::value<endpoint_t>(&(context->join_addr)), "join to an existing address.")
    ("bootstrap,b", "bootstrap peer to create a new chord ring.")
    ("uuid,u,id", po::value<uuid_t>(&(context->uuid)), "client uuid.")
    ("bind", po::value<endpoint_t>(&(context->bind_addr)), "bind address that is promoted to clients.")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, desc), vm);
  po::notify(vm);

  //--- help
  if(vm.count("help")) {
    std::cout << desc << std::endl;
    exit(1);
  }

  //--- validate
  if( (!vm.count("join") && !vm.count("bootstrap")) ||
      (vm.count("join") && vm.count("bootstrap")) ) {
    fatal << "please specify either a join address or the bootstrap flag\n\n"
          << desc << std::endl;
    exit(2);
  }

  //--- join
  if(vm.count("join")) {
    LOG(trace) << "[option-join] " << vm["join"].as<std::string>();
  }

  //--- bootstrap
  if(vm.count("bootstrap")) {
    LOG(trace) << "[option-bootstrap] " << "true";
    context->bootstrap = true;
  }

  //--- client-id
  if(vm.count("uuid")) {
    LOG(trace) << "[option-uuid] " << vm["uuid"].as<uuid_t>() << std::endl;
  }
}

int main(int argc, char* argv[]) {

  std::shared_ptr<Context> context(new Context);
  parse_program_options(argc, argv, context);

  ChordPeer peer(context);
  //ChordClient client(context);
  //LOG(debug) << "joining " << context->join_addr;
  //ChordServiceImpl service;

  //std::thread service_thread(&ChordServiceImpl::start_server, std::ref(service), "localhost:50050");
  //std::thread client_thread(&ChordClient::join, std::ref(client), "localhost:50050");

  //LOG(debug) << "waiting for client and service to finish.";

  //client.connect("localhost:50050");
  //client.connect(grpc::CreateChannel("localhost:50050", grpc::InsecureChannelCredentials()));


  std::this_thread::sleep_for(std::chrono::milliseconds(50000));

  //client_thread.join();
  //service_thread.join();
  return 0;
}
