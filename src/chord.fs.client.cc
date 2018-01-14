#include <memory>
#include <fstream>

#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

#include "chord.common.h"
#include "chord.crypto.h"
#include "chord.file.h"
#include "chord.fs.client.h"
#include "chord.log.h"
#include "chord.peer.h"
#include "chord.router.h"

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
using chord::fs::MetaResponse;
using chord::fs::MetaRequest;

using namespace std;
using namespace chord::common;
using namespace chord::file;

namespace chord {
namespace fs {

Client::Client(Context* context, ChordFacade* chord)
    : context{context}, chord{chord} {
  //--- default stub factory
  make_stub = [&](const endpoint_t &endpoint) {
    return chord::fs::Filesystem::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  };
}

Client::Client(Context* context, ChordFacade* chord, StubFactory make_stub)
    : context{context}, chord{chord}, make_stub{make_stub} {
}

Status Client::put(const chord::uri &uri, std::istream &istream) {
  auto hash = chord::crypto::sha256(uri);
  auto endpoint = chord->successor(hash).endpoint();

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

void Client::add_metadata(MetaRequest& req, const chord::path& parent_path) {
  for (const auto &c : parent_path.contents()) {
    auto *item = req.add_files();
    item->set_type(is_directory(c)
                       ? value_of(type::directory)
                       : is_regular_file(c) ? value_of(type::regular)
                                            : value_of(type::unknown));
    item->set_filename(c.filename());
    // TODO set permission
  }
}

grpc::Status Client::meta(const chord::uri &uri, const Action &action) {
  //--- find responsible node
  auto meta_uri = uri::builder{uri.scheme(), uri.path().parent_path().canonical()}.build();
  auto hash = chord::crypto::sha256(meta_uri);
  auto endpoint = chord->successor(hash).endpoint();

  CLIENT_LOG(trace, meta) << meta_uri << " (" << hash << ")";

  ClientContext clientContext;
  MetaResponse res;
  MetaRequest req;

  auto path = uri.path().canonical();

  req.set_id(hash);
  req.set_uri(meta_uri);
  {
    auto data = req.metadata();
    data.set_type(is_directory(path)
                      ? value_of(type::directory)
                      : is_regular_file(path) ? value_of(type::regular)
                                              : value_of(type::unknown));
    data.set_filename(path.filename());
  }

  switch (action) {
    case Action::ADD:
      req.set_action(ADD); 
      if(is_directory(path)) add_metadata(req, meta_uri.path());
      break;
    case Action::DEL:
      req.set_action(DEL); 
      break;
  }

  auto status = make_stub(endpoint)->meta(&clientContext, req, &res);

  return status;
}

Status Client::get(const chord::uri &uri, std::ostream &ostream) {
  auto hash = chord::crypto::sha256(uri);
  auto endpoint = chord->successor(hash).endpoint();

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
  auto endpoint = chord->successor(hash).endpoint();

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
