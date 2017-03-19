#include "chord.context.h"
#include "chord.router.h"

Context::Context() {
  //router = new Router(this);
  router = std::make_shared<Router>(this);
}

Context Context::set_uuid(const uuid_t& uuid) {
    _uuid = uuid;
    if( router != nullptr ) {
      router->reset();
    }
    return *this;
}
