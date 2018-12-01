#include "chord.context.h"
#include "chord.router.h"
#include "chord.uuid.h"

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
} //namespace chord
