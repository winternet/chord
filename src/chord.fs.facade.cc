#include "chord.fs.facade.h"

using namespace std;

namespace chord {
namespace fs {

Facade::Facade(Context& context, ChordFacade* chord)
    : fs_client{make_unique<fs::Client>(context, chord)},
      fs_service{make_unique<fs::Service>(context, chord)}
{}

::grpc::Service* Facade::grpc_service() {
  return fs_service.get();
}

void Facade::put(const chord::uri &uri, istream &istream) {
  auto status = fs_client->put(uri, istream);
  if(!status.ok()) throw__grpc_exception("failed to put " + to_string(uri), status);

  //status = fs_client->meta(uri, Client::Action::ADD);
  ////TODO handle error
  //if(!status.ok()) throw__grpc_exception("failed to safe metadata " + to_string(uri), status);
}

void Facade::get(const chord::uri &uri, ostream &ostream) {
  auto status = fs_client->get(uri, ostream);
  if(!status.ok()) throw__grpc_exception("failed to get " + to_string(uri), status);
}

void Facade::dir(const chord::uri &uri, iostream &iostream) {
  auto status = fs_client->dir(uri, iostream);
  if(!status.ok()) throw__grpc_exception("failed to dir " + to_string(uri), status);
}

void Facade::del(const chord::uri &uri) {
  auto status = fs_client->del(uri);
  if(!status.ok()) throw__grpc_exception("failed to del file " + to_string(uri), status);

  //status = fs_client->meta(uri, Client::Action::DEL);
  //if(!status.ok()) throw__grpc_exception("failed to del metadata " + to_string(uri), status);
}

}
}
