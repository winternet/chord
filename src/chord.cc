#include <thread>
#include <chrono>
#include <iostream>

#include <boost/log/trivial.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "chord.client.cc"
#include "chord.service.cc"

int main() {

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
