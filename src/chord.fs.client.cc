#include "chord.fs.client.h"

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/impl/codegen/status_code_enum.h>
#include <grpcpp/impl/codegen/sync_stream.h>
#include <grpcpp/security/credentials.h>
#include <array>
#include <fstream>
#include <memory>
#include <cstddef>
#include <string>

#include "chord_fs.grpc.pb.h"
#include "chord_fs.pb.h"

#include "chord.context.h"
#include "chord.facade.h"
#include "chord.crypto.h"
#include "chord.exception.h"
#include "chord.fs.context.metadata.h"
#include "chord.fs.metadata.builder.h"
#include "chord.fs.metadata.h"
#include "chord.fs.replication.h"
#include "chord.fs.type.h"
#include "chord.log.factory.h"
#include "chord.log.h"
#include "chord.node.h"
#include "chord.uuid.h"
#include "chord.fs.client.options.h"

using grpc::ClientContext;
using grpc::Status;
using grpc::StatusCode;
using grpc::ClientWriter;
using grpc::ClientReader;

using chord::fs::PutRequest;
using chord::fs::GetResponse;
using chord::fs::GetRequest;
using chord::fs::MetaResponse;
using chord::fs::MetaRequest;

using namespace std;
using namespace chord::file;

namespace chord {
namespace fs {

Client::Client(Context &context, ChordFacade *chord, IMetadataManager* metadata_mgr, ChannelPool* channel_pool)
    : context{context},
      chord{chord},
      channel_pool{channel_pool},
      metadata_mgr{metadata_mgr},
      make_stub{//--- default stub factory
                [](const endpoint& endpoint) {
                  return chord::fs::Filesystem::NewStub(grpc::CreateChannel(
                      endpoint, grpc::InsecureChannelCredentials()));
                }},
      logger{context.logging.factory().get_or_create(logger_name)} {}

Client::Client(Context &context, ChordFacade *chord, StubFactory make_stub)
    : context{context},
      chord{chord},
      make_stub{make_stub},
      logger{context.logging.factory().get_or_create(logger_name)} {}

void Client::init_context(ClientContext& client_context, const client::options& options) {
  if(options.source)
    ContextMetadata::add_src(client_context, *options.source);
  ContextMetadata::add_rebalance(client_context, options.rebalance);
}

Status Client::put(const chord::uri& uri, const chord::path& source, const client::options& options) {
  const auto hash = chord::crypto::sha256(uri);
  const auto node = chord->successor(hash);
  return put(node, uri, source, options);
}

Status Client::put(const chord::node& node, const chord::uri& uri, const chord::path& source, const client::options& options) {
  try {
    std::ifstream file;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    file.open(source, std::fstream::binary);

    return put(node, uri, file, options);
  } catch (const std::ios_base::failure& exception) {
    return Status(grpc::StatusCode::CANCELLED, "failed to put", exception.what());
  }
}

Status Client::put(const chord::node& node, const chord::uri &uri, istream &istream, const client::options& options) {

  if(node == context.node() && options.source && *options.source == context.uuid()) {
    return Status{StatusCode::ALREADY_EXISTS, "trying to issue request from self - aborting."};
  }

  //TODO make configurable
  constexpr size_t len = 512*1024; // 512k
  std::array<char, len> buffer;

  ClientContext clientContext;
  init_context(clientContext, options);
  ContextMetadata::add(clientContext, options.replication);
  ContextMetadata::add(clientContext, uri);

  if(metadata_mgr->exists(uri)) {
    const auto metadata_set = metadata_mgr->get(uri);
    if(fs::is_regular_file(metadata_set)) {
      ContextMetadata::add(clientContext, metadata_set.begin()->file_hash);
    } else {
      logger->warn("[put] failed to handle metadata for uri {}: multiple entries ({}) - aborting.", uri, metadata_set.size());
      return Status::CANCELLED;
    }
  }

  PutResponse res;

  auto stub = Filesystem::NewStub(grpc::CreateChannel(node.endpoint, grpc::InsecureChannelCredentials()));
  unique_ptr<ClientWriter<PutRequest> > writer(stub->put(&clientContext, &res));

  writer->WaitForInitialMetadata();
  const bool file_hash_equal = ContextMetadata::file_hash_equal_from(clientContext);

  if(file_hash_equal) {
    logger->info("[put] file hash equals - skip upload.");
  }

  for(size_t read=0, offset=0; !file_hash_equal && istream && (read = static_cast<size_t>(istream.readsome(buffer.data(), len))) > 0;) {
      PutRequest req;
      req.set_data(buffer.data(), read);
      req.set_offset(offset);
      req.set_size(read);

      offset += read;

      if (!writer->Write(req)) {
        break;
        //throw__exception("broken stream.");
      }
  }
  writer->WritesDone();

  return writer->Finish();
}

Status Client::put(const chord::uri &uri, istream &istream, const client::options& options) {
  const auto hash = chord::crypto::sha256(uri);
  const auto node = chord->successor(hash);
  logger->trace("[put] {} ({})", uri, hash);
  return put(node, uri, istream, options);
}

grpc::Status Client::meta(const chord::node& target, const chord::uri &uri, const Action &action, set<Metadata>& metadata, const client::options& options) {
  //--- find responsible node for uri.path()
  const auto path = uri.path().canonical();
  const auto meta_uri = uri::builder{uri.scheme(), path}.build();
  const auto hash = chord::crypto::sha256(meta_uri);

  logger->trace("[meta] {} ({})", meta_uri, hash);

  ClientContext clientContext;
  init_context(clientContext, options);
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
      if(m.node_ref) {
        auto node_ref = data->mutable_node_ref();
        node_ref->set_uuid(m.node_ref->uuid.string());
        node_ref->set_endpoint(m.node_ref->endpoint);
      }
      if(m.file_hash) {
        data->set_file_hash(*m.file_hash);
      }
      data->set_replication_idx(m.replication.index);
      data->set_replication_cnt(m.replication.count);
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
      return dir(uri, metadata, options);
  }

  const auto status = make_stub(target.endpoint)->meta(&clientContext, req, &res);

  return status;
}

grpc::Status Client::meta(const chord::uri &uri, const Action &action, std::set<Metadata>& m, const client::options& options) {
  const auto hash = chord::crypto::sha256(uri);
  const auto node = chord->successor(hash);
  return meta(node, uri, action, m, options);
}

Status Client::del(const chord::node& node, const DelRequest* req, const client::options& options) {
  const auto endpoint = node.endpoint;
  ClientContext clientContext;
  init_context(clientContext, options);
  DelResponse res;
  return make_stub(endpoint)->del(&clientContext, *req, &res);
}

Status Client::del(const chord::node& node, const chord::uri &uri, const bool recursive, const client::options& options) {
  const auto hash = chord::crypto::sha256(uri);
  const auto endpoint = node.endpoint;

  logger->trace("[del] {} ({}) -> {}", uri, hash, endpoint);

  ClientContext clientContext;
  init_context(clientContext, options);
  DelResponse res;
  DelRequest req;

  req.set_id(hash);
  req.set_uri(uri);
  req.set_recursive(recursive);

  const auto status = make_stub(endpoint)->del(&clientContext, req, &res);

  return status;
}

// currently only files are supported
Status Client::del(const chord::uri &uri, const bool recursive, const client::options& options) {
  const auto hash = chord::crypto::sha256(uri);
  const auto succ = chord->successor(hash);
  return del(succ, uri, recursive, options);
}

grpc::Status Client::dir(const chord::uri &uri, std::set<Metadata> &metadata, const client::options& options) {
  //--- find responsible node
  const auto meta_uri = uri::builder{uri.scheme(), uri.path().canonical()}.build();
  const auto hash = chord::crypto::sha256(meta_uri);
  const auto endpoint = chord->successor(hash).endpoint;

  logger->trace("[dir] {} ({})", meta_uri, hash);

  ClientContext clientContext;
  init_context(clientContext, options);

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

  logger->trace("[get] {} ({})", uri, hash);

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
  const auto node = chord->successor(hash);
  return get(uri, node, ostream);
}

} // namespace fs
} // namespace chord
