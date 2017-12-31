#pragma once

#include "chord.uri.h"
#include "chord.fs.client.h"
#include "chord.fs.service.h"

namespace chord {
namespace fs {

class Facade {
private:
  std::unique_ptr<chord::fs::Client> fs_client;
  std::unique_ptr<chord::fs::Service> fs_service;

public:
  Facade(Context* context, ChordFacade* chord);

  ::grpc::Service* service();

  void put(const chord::uri &uri, std::istream &istream);

  void get(const chord::uri &uri, std::ostream &ostream);

  void notify(const std::string &uri);
};

} //namespace fs
} //namespace chord
