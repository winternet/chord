#include "chord.fs.facade.h"


using namespace std;

namespace chord {
namespace fs {

Facade::Facade(Context* context, ChordFacade* chord)
    : fs_client{make_unique<fs::Client>(context, chord)},
      fs_service{make_unique<fs::Service>(context)}
{}

::grpc::Service* Facade::service() {
  return fs_service.get();
}

void Facade::put(const string &uri, istream &istream) {
  auto status = fs_client->put(uri, istream);
  if(!status.ok()) throw chord::exception("failed to put " + uri, status);
}

void Facade::get(const string &uri, ostream &ostream) {
  auto status = fs_client->get(uri, ostream);
  if(!status.ok()) throw chord::exception("failed to get " + uri, status);
}

}
}
