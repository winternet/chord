#pragma once

#include "chord.fs.client.h"
#include "chord.fs.service.h"
#include "chord.uri.h"

namespace chord {
namespace fs {

class Facade {
private:
  std::unique_ptr<Client> fs_client;
  std::unique_ptr<Service> fs_service;

public:
  Facade(Context& context, ChordFacade* chord);

  ::grpc::Service* grpc_service();

  void put(const chord::uri &uri, std::istream &istream);

  void get(const chord::uri &uri, std::ostream &ostream);

  void dir(const chord::uri &uri, std::iostream &iostream);

  void del(const chord::uri &uri);

};

} //namespace fs
} //namespace chord
