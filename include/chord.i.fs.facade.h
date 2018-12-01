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

  virtual void put(const chord::path& source, const chord::uri& target, Replication repl = Replication()) = 0;

  virtual void get(const chord::uri& source, const chord::path& target) =0;

  virtual void dir(const chord::uri& uri, std::iostream& iostream) =0;

  virtual void del(const chord::uri& uri) =0;

};

} //namespace fs
} //namespace chord
