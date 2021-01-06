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

  virtual grpc::Status put(const chord::path& source, const chord::uri& target, Replication repl = {}) = 0;

  virtual grpc::Status get(const chord::uri& source, const chord::path& target) =0;

  virtual grpc::Status mkdir(const chord::uri&, Replication) =0;
  virtual grpc::Status exists(const chord::uri& uri) =0;
  virtual grpc::Status move(const chord::uri& source, const chord::uri& target, const bool force=false) =0;

  virtual grpc::Status dir(const chord::uri& uri, std::set<Metadata>&) =0;
  virtual grpc::Status dir(const chord::uri& uri, std::ostream& ostream) =0;

  virtual grpc::Status del(const chord::uri& uri, const bool recursive=false) =0;

};

} //namespace fs
} //namespace chord
