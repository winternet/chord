#include "chord.uuid.h"

typedef std::string endpoint_t;

struct Router {

  std::map<uuid_t, endpoint_t> routes;


};
