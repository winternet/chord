#include <memory>
#include <fstream>

#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

#include "chord.log.h"
#include "chord.peer.h"
#include "chord.fs.client.h"
#include "chord.router.h"

#include "chord.common.h"
#include "chord.crypto.h"

#define log(level) LOG(level) << "[client] "
#define CLIENT_LOG(level, method) LOG(level) << "[client][" << #method << "] "

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientWriter;
using grpc::ClientReader;

using chord::common::Header;
using chord::common::RouterEntry;

using chord::fs::PutResponse;
using chord::fs::PutRequest;
using chord::fs::GetResponse;
using chord::fs::GetRequest;

using namespace std;
using namespace chord::common;

namespace chord {
namespace fs {

Client::Client(std::shared_ptr<Context> context, std::shared_ptr<Peer> peer)
    : context{context}, peer{peer} {
  //--- default stub factory
  make_stub = [&](const endpoint_t &endpoint) {
    return chord::fs::Filesystem::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  };
}

Client::Client(std::shared_ptr<Context> context, std::shared_ptr<Peer> peer, StubFactory make_stub)
    : context{context}, peer{peer}, make_stub{make_stub} {
}

Status Client::put(const std::string &uri, std::istream &istream) {
  auto hash = chord::crypto::sha256(uri);
  auto endpoint = peer->successor(hash).endpoint();

  CLIENT_LOG(trace, put) << uri << " (" << hash << ")";

  //TODO make configurable
  constexpr size_t len = 512*1024; // 512k
  char buffer[len];
  size_t offset = 0,
      read = 0;

  ClientContext clientContext;
  PutResponse res;
  // cannot be mocked since make_stub returns unique_ptr<StubInterface> (!)
  auto stub = Filesystem::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  std::unique_ptr<ClientWriter<PutRequest> > writer(stub->put(&clientContext, &res));

  do {
    read = istream.readsome(buffer, len);
    if (read <= 0) {
      break;
    }

    PutRequest req;
    req.set_id(hash);
    req.set_data(buffer, read);
    req.set_offset(offset);
    req.set_size(read);
    req.set_uri(uri);
    offset += read;

    if (!writer->Write(req)) {
      throw chord::exception("broken stream.");
    }

  } while (read > 0);

  writer->WritesDone();
  return writer->Finish();
}

Status Client::get(const std::string &uri, std::ostream &ostream) {
  auto hash = chord::crypto::sha256(uri);
  auto endpoint = peer->successor(hash).endpoint();

  CLIENT_LOG(trace, get) << uri << " (" << hash << ")";

  ClientContext clientContext;
  GetResponse res;
  GetRequest req;

  req.set_id(hash);
  req.set_uri(uri);

  // cannot be mocked since make_stub returns unique_ptr<StubInterface> (!)
  auto stub = Filesystem::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  unique_ptr<ClientReader<GetResponse> > reader(stub->get(&clientContext, req));

  while (reader->Read(&res)) {
    ostream.write(res.data().data(), res.size());
  }

  return reader->Finish();
}

/*
Status Client::dir(const std::string& uri, std::ostream& ostream) {
  auto hash = chord::crypto::sha256(uri);
  auto endpoint = peer->successor(hash).endpoint();

  CLIENT_LOG(trace, get) << uri << " (" << hash << ")";

  ClientContext clientContext;
  GetResponse res;
  GetRequest req;

  req.set_id(hash);
  req.set_uri(uri);

  // cannot be mocked since make_stub returns unique_ptr<StubInterface> (!)
  auto stub = Filesystem::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  unique_ptr<ClientReader<GetResponse> > reader(stub->get(&clientContext, req));

  while(reader->Read(&res)) {
    ostream.write(res.data().data(), res.size());
  }

  return reader->Finish();
}
*/

} // namespace fs
} // namespace chord
