#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

typedef boost::uuid::uuid uuid_t;
typedef std::string endpoint_t;

struct Router {

  std::map<uuid_t, endpoint_t> routes;


};
