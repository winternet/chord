#include "chord.context.h"
#include "chord.router.h"
#include <memory>

using namespace std;

Context::Context() {
  //router = new Router(this);
  router = make_shared<Router>(this);
}

Context Context::set_uuid(const uuid_t& uuid) {
  _uuid = uuid;
  if( router != nullptr ) {
    router->reset();
  }
  return *this;
}
