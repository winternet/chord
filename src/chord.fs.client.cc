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
#include "chord.fs.replication.h"
#include "chord.log.h"
#include "chord.peer.h"
#include "chord.router.h"
#include "chord.utils.h"
#include "chord.fs.context.metadata.h"
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientWriter;
using grpc::ClientReader;

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

Client::Client(Context &context, ChordFacade *chord)
    : context{context},
      chord{chord},
      make_stub{//--- default stub factory
                [](const endpoint_t &endpoint) {
                  return chord::fs::Filesystem::NewStub(grpc::CreateChannel(
                      endpoint, grpc::InsecureChannelCredentials()));
                }},
      logger{context.logging.factory().get_or_create(logger_name)} {}

Client::Client(Context &context, ChordFacade *chord, StubFactory make_stub)
    : context{context},
      chord{chord},
      make_stub{make_stub},
      logger{context.logging.factory().get_or_create(logger_name)} {}

Status Client::put(const chord::uri& uri, const chord::path& source, Replication repl) {
  const auto hash = chord::crypto::sha256(uri);
  const auto node = make_node(chord->successor(hash));
  return put(node, uri, source, repl);
}

Status Client::put(const chord::node& node, const chord::uri& uri, const chord::path& source, Replication repl) {
  try {
    std::ifstream file;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    file.open(source, std::fstream::binary);

    return put(node, uri, file, repl);
  } catch (const std::ios_base::failure& exception) {
    return Status(grpc::StatusCode::CANCELLED, "failed to put", exception.what());
  }
}

Status Client::put(const chord::node& node, const chord::uri &uri, istream &istream, Replication repl) {

  //TODO make configurable
  constexpr size_t len = 512*1024; // 512k
  array<char, len> buffer;
  size_t offset = 0, 
         read = 0;

  ClientContext clientContext;
  ContextMetadata::add(clientContext, repl);
  PutResponse res;
  // cannot be mocked since make_stub returns unique_ptr<StubInterface> (!)
  auto stub = Filesystem::NewStub(grpc::CreateChannel(node.endpoint, grpc::InsecureChannelCredentials()));
  unique_ptr<ClientWriter<PutRequest> > writer(stub->put(&clientContext, &res));

  do {
    read = static_cast<size_t>(istream.readsome(buffer.data(), len));
    if (read == 0) break;

    PutRequest req;
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

Status Client::put(const chord::uri &uri, istream &istream, Replication repl) {
  const auto hash = chord::crypto::sha256(uri);
  const auto node = make_node(chord->successor(hash));
  logger->trace("put {} ({})", uri, hash);
  return put(node, uri, istream, repl);
}

void Client::add_metadata(MetaRequest& req, const chord::path& parent_path) {
  for (const auto &c : parent_path.contents()) {
    auto *item = req.add_metadata();
    item->set_type(chord::file::is_directory(c)
                       ? value_of(type::directory)
                       : is_regular_file(c) ? value_of(type::regular)
                                            : value_of(type::unknown));
    item->set_filename(c.filename());
    // TODO set permission
  }
}

grpc::Status Client::meta(const chord::node& target, const chord::uri &uri, const Action &action, set<Metadata>& metadata) {
  //--- find responsible node for uri.path()
  const auto path = uri.path().canonical();
  const auto meta_uri = uri::builder{uri.scheme(), path}.build();
  const auto hash = chord::crypto::sha256(meta_uri);

  logger->trace("meta {} ({})", meta_uri, hash);

  ClientContext clientContext;
  MetaResponse res;
  MetaRequest req;

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
      if(m.replication) {
        data->set_replication_idx(m.replication->index);
        data->set_replication_cnt(m.replication->count);
      }
    }
  }

  const auto local_path = context.data_directory / path;
  switch (action) {
    case Action::ADD:
      req.set_action(ADD); 
      break;
    case Action::DEL:
      req.set_action(DEL); 
      break;
    case Action::DIR:
      return dir(uri, metadata);
  }

  const auto status = make_stub(target.endpoint)->meta(&clientContext, req, &res);

  return status;
}

grpc::Status Client::meta(const chord::uri &uri, const Action &action, std::set<Metadata>& m) {
  const auto hash = chord::crypto::sha256(uri);
  const auto node = chord::common::make_node(chord->successor(hash));
  return meta(node, uri, action, m);
}
grpc::Status Client::meta(const chord::uri &uri, const Action &action) {
  std::set<Metadata> m;
  const auto hash = chord::crypto::sha256(uri);
  const auto node = chord::common::make_node(chord->successor(hash));
  return meta(node, uri, action, m);
}

Status Client::del(const chord::node& node, const DelRequest* req) {
  const auto endpoint = node.endpoint;
  ClientContext clientContext;
  DelResponse res;
  return make_stub(endpoint)->del(&clientContext, *req, &res);
}
Status Client::del(const chord::node& node, const chord::uri &uri, const bool recursive, const Replication repl) {
  const auto hash = chord::crypto::sha256(uri);
  const auto endpoint = node.endpoint;

  logger->trace("del {} ({})", uri, hash);

  ClientContext clientContext;
  DelResponse res;
  DelRequest req;

  req.set_id(hash);
  req.set_uri(uri);
  req.set_recursive(recursive);

  const auto status = make_stub(endpoint)->del(&clientContext, req, &res);

  return status;
}

// currently only files are supported
Status Client::del(const chord::uri &uri, const bool recursive, const Replication repl) {
  const auto hash = chord::crypto::sha256(uri);
  const auto succ = chord::common::make_node(chord->successor(hash));
  return del(succ, uri, recursive, repl);
}

grpc::Status Client::dir(const chord::uri &uri, std::set<Metadata> &metadata) {
  //--- find responsible node
  const auto meta_uri = uri::builder{uri.scheme(), uri.path().canonical()}.build();
  const auto hash = chord::crypto::sha256(meta_uri);
  const auto endpoint = chord->successor(hash).endpoint();

  logger->trace("dir {} ({})", meta_uri, hash);

  ClientContext clientContext;
  MetaResponse res;
  MetaRequest req;

  const auto path = uri.path().canonical();

  req.set_id(hash);
  req.set_uri(meta_uri);
  req.set_action(DIR);

  const auto status = make_stub(endpoint)->meta(&clientContext, req, &res);

  const auto meta_res = MetadataBuilder::from(res);
  metadata.insert(meta_res.begin(), meta_res.end());

  return status;
}

Status Client::get(const chord::uri& uri, const chord::path& path) {
  std::ofstream ofile;
  ofile.exceptions(ofstream::failbit | ofstream::badbit);
  ofile.open(path, fstream::binary);
  return get(uri, ofile);
}

Status Client::get(const chord::uri& uri, const chord::node& node, const chord::path& path) {
  std::ofstream ofile;
  ofile.exceptions(ofstream::failbit | ofstream::badbit);
  ofile.open(path, fstream::binary);
  return get(uri, node, ofile);
}

Status Client::get(const chord::uri &uri, const chord::node& node, std::ostream &ostream) {
  const auto hash = chord::crypto::sha256(uri);

  logger->trace("get {} ({})", uri, hash);

  ClientContext clientContext;
  GetResponse res;
  GetRequest req;

  req.set_id(hash);
  req.set_uri(uri);

  // cannot be mocked since make_stub returns unique_ptr<StubInterface> (!)
  const auto stub = Filesystem::NewStub(grpc::CreateChannel(node.endpoint, grpc::InsecureChannelCredentials()));
  unique_ptr<ClientReader<GetResponse> > reader(stub->get(&clientContext, req));

  while (reader->Read(&res)) {
    ostream.write(res.data().data(), static_cast<std::streamsize>(res.size()));
  }

  return reader->Finish();
}

Status Client::get(const chord::uri &uri, ostream &ostream) {
  const auto hash = chord::crypto::sha256(uri);
  const auto uuid = chord->successor(hash).uuid();
  const auto endpoint = chord->successor(hash).endpoint();
  return get(uri, {uuid, endpoint}, ostream);
}

} // namespace fs
} // namespace chord
