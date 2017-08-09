#include "chord.context.h"
#include "chord.router.h"
#include <memory>

using namespace std;

Context::Context() { }

Context Context::set_uuid(const uuid_t& uuid) {
  _uuid = uuid;
  if( router != nullptr ) {
    router->reset();
  }
  return *this;
}
Context Context::set_router(Router* router) {
  this->router = router;
  return *this;
}
