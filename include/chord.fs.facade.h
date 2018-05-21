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

private:
 void put_file(const chord::path& source, const chord::uri& target);
 void get_file(const chord::uri& source, const chord::path& target);

public:
  Facade(Context& context, ChordFacade* chord);

  ::grpc::Service* grpc_service();

  void put(const chord::path& source, const chord::uri& target);

  void get(const chord::uri& source, const chord::path& target);

  void dir(const chord::uri& uri, std::iostream& iostream);

  void del(const chord::uri& uri);
};

} //namespace fs
} //namespace chord
