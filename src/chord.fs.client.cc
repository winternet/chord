#include <grpc++/channel.h>
#include <grpc++/create_channel.h>
#include <array>
#include <fstream>
#include <memory>

#include "chord.common.h"
#include "chord.crypto.h"
#include "chord.file.h"
#include "chord.fs.client.h"
#include "chord.fs.metadata.builder.h"
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

Client::Client(Context &context, ChordFacade* chord)
    : context{context}, chord{chord} {
  //--- default stub factory
  make_stub = [&](const endpoint_t &endpoint) {
    return chord::fs::Filesystem::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  };
}

Client::Client(Context &context, ChordFacade* chord, StubFactory make_stub)
    : context{context}, chord{chord}, make_stub{make_stub} {
}


Status Client::put(const chord::uri &uri, istream &istream) {
  const auto hash = chord::crypto::sha256(uri);
  const auto endpoint = chord->successor(hash).endpoint();

  CLIENT_LOG(trace, put) << uri << " (" << hash << ")";

  //TODO make configurable
  constexpr size_t len = 512*1024; // 512k
  array<char, len> buffer;
  size_t offset = 0,
         read = 0;

  ClientContext clientContext;
  PutResponse res;
  // cannot be mocked since make_stub returns unique_ptr<StubInterface> (!)
  auto stub = Filesystem::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));
  unique_ptr<ClientWriter<PutRequest> > writer(stub->put(&clientContext, &res));

  do {
    read = istream.readsome(buffer.data(), len);
    if (read == 0) break;

    PutRequest req;
    req.set_id(hash);
    req.set_data(buffer.data(), read);
    req.set_offset(offset);
    req.set_size(read);
    req.set_uri(uri);
    offset += read;

    if (!writer->Write(req)) {
      throw__exception("broken stream.");
    }

  } while (read > 0);

  writer->WritesDone();

  return writer->Finish();
}

void Client::add_metadata(MetaRequest& req, const chord::path& parent_path) {
  for (const auto &c : parent_path.contents()) {
    auto *item = req.add_metadata();
    item->set_type(is_directory(c)
                       ? value_of(type::directory)
                       : is_regular_file(c) ? value_of(type::regular)
                                            : value_of(type::unknown));
    item->set_filename(c.filename());
    // TODO set permission
  }
}

grpc::Status Client::meta(const chord::uri &uri, const Action &action, const set<Metadata>& metadata) {
  //--- find responsible node for uri.parent_path()
  const auto path = uri.path().canonical();
  const auto meta_uri = uri::builder{uri.scheme(), path}.build();
  const auto hash = chord::crypto::sha256(meta_uri);
  const auto endpoint = chord->successor(hash).endpoint();

  CLIENT_LOG(trace, meta) << meta_uri << " (" << hash << ")";

  ClientContext clientContext;
  MetaResponse res;
  MetaRequest req;

  auto local_path = context.data_directory / path;

  req.set_id(hash);
  req.set_uri(meta_uri);
  {
    for(const auto& m:metadata) {
      auto data = req.add_metadata();
      data->set_filename(m.name);
      data->set_type(value_of(m.file_type));
      data->set_permissions(value_of(m.permissions));
      data->set_owner(m.owner);
      data->set_group(m.group);
    }
  }

  switch (action) {
    case Action::ADD:
      req.set_action(ADD); 
      if(chord::file::is_directory(local_path)) add_metadata(req, local_path);
      break;
    case Action::DEL:
      req.set_action(DEL); 
      break;
    case Action::DIR:
      return dir(uri, cout);
  }

  auto status = make_stub(endpoint)->meta(&clientContext, req, &res);

  return status;
}

grpc::Status Client::meta(const chord::uri &uri, const Action &action) {
  return meta(uri, action, {});
}

Status Client::del(const chord::uri &uri) {
  const auto hash = chord::crypto::sha256(uri);
  const auto endpoint = chord->successor(hash).endpoint();

  CLIENT_LOG(trace, del) << uri << " (" << hash << ")";

  ClientContext clientContext;
  DelResponse res;
  DelRequest req;

  req.set_id(hash);
  req.set_uri(uri);

  auto status = make_stub(endpoint)->del(&clientContext, req, &res);

  return status;
}

grpc::Status Client::dir(const chord::uri &uri, ostream &stream) {
  //--- find responsible node
  const auto meta_uri = uri::builder{uri.scheme(), uri.path().canonical()}.build();
  const auto hash = chord::crypto::sha256(meta_uri);
  const auto endpoint = chord->successor(hash).endpoint();

  CLIENT_LOG(trace, dir) << meta_uri << " (" << hash << ")";

  ClientContext clientContext;
  MetaResponse res;
  MetaRequest req;

  auto path = uri.path().canonical();

  req.set_id(hash);
  req.set_uri(meta_uri);
  req.set_action(DIR);

  auto status = make_stub(endpoint)->meta(&clientContext, req, &res);

  stream << MetadataBuilder::from(res);

  return status;
}

Status Client::get(const chord::uri &uri, ostream &ostream) {
  const auto hash = chord::crypto::sha256(uri);
  const auto endpoint = chord->successor(hash).endpoint();

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

} // namespace fs
} // namespace chord
