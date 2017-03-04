#include <thread>
#include <chrono>
#include <iostream>

#include <boost/log/trivial.hpp>

#include <boost/program_options.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "chord.log.h"
#include "chord.client.h"
#include "chord.service.h"

namespace po = boost::program_options;

void parse_program_options(int ac, char* av[]) {
  po::options_description desc("[program options]");

  desc.add_options()
    ("help,h", "produce help message")
    ("join,j", po::value<std::string>(), "join to an existing address.")
    ("bootstrap,b", "bootstrap peer to create a new chord ring.")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, desc), vm);
  po::notify(vm);

  if(vm.count("help")) {
    std::cout << desc << std::endl;
    exit(1);
  }

  if(vm.count("join")) {
    std::cout << "Compression was set to "
      << vm["compression"].as<std::string>() << std::endl;
  } else {
    std::cout << "Compression level was not set." << std::endl;
  }
}

int main(int argc, char* argv[]) {

  parse_program_options(argc, argv);

  std::shared_ptr<Context> context(new Context());

  ChordClient client(context);
  //ChordServiceImpl service;

  //std::thread service_thread(&ChordServiceImpl::start_server, std::ref(service), "localhost:50050");
  //std::thread client_thread(&ChordClient::join, std::ref(client), "localhost:50050");

  BOOST_LOG_TRIVIAL(debug) << "waiting for client and service to finish.";

  client.connect("localhost:50050");
  //client.connect(grpc::CreateChannel("localhost:50050", grpc::InsecureChannelCredentials()));


  std::this_thread::sleep_for(std::chrono::milliseconds(50000));

  //client_thread.join();
  //service_thread.join();
  return 0;
}
