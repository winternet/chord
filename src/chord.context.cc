#include "chord.context.h"
#include "chord.router.h"
#include "chord.uuid.h"
#include "chord.exception.h"

using namespace std;

namespace chord {

void Context::set_uuid(const uuid_t &uuid) {
  _uuid = uuid;
  if (router!=nullptr) {
    router->reset();
  }
}

Context Context::set_router(Router *t_router) {
  router = t_router;
  return *this;
}

void Context::validate(const Context& context) {
    // validate
    if(context.bind_addr == context.join_addr) {
      throw__exception("Bind address must not be equal to join address.");
    }
    if(context.meta_directory == context.data_directory) {
      throw__exception("Meta directory must not be equal to data directory.");
    }
}
} //namespace chord
