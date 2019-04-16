#pragma once

#include <set>

#include "chord.fs.client.h"
#include "chord.fs.service.h"
#include "chord.log.h"
#include "chord.uri.h"

namespace chord {
namespace fs {

class IFacade {

 public:
  virtual ~IFacade() = default;

  virtual grpc::Status put(const chord::path& source, const chord::uri& target, Replication repl = Replication()) = 0;

  virtual grpc::Status get(const chord::uri& source, const chord::path& target) =0;

  virtual grpc::Status dir(const chord::uri& uri, std::iostream& iostream) =0;

  virtual grpc::Status del(const chord::uri& uri, const bool recursive=false) =0;

};

} //namespace fs
} //namespace chord
