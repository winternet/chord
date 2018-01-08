#include "chord.context.h"
#include "chord.router.h"

using namespace std;

namespace chord {

void Context::set_uuid(const uuid_t &uuid) {
  _uuid = uuid;
  if (router!=nullptr) {
    router->reset();
  }
}

Context Context::set_router(Router *router) {
  this->router = router;
  return *this;
}
} //namespace chord
