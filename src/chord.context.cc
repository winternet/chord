#include "chord.context.h"
#include "chord.router.h"

Context::Context() {
  router = new Router(this);
}
