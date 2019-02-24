#include <iostream>
#include <fstream>
#include <thread>
#include <experimental/filesystem>

#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include "chord.log.h"
#include "chord.uri.h"
#include "chord.file.h"
#include "chord.context.h"
#include "chord_fs.grpc.pb.h"
#include "chord.i.callback.h"

#include "chord.common.h"
#include "chord.client.h"
#include "chord.fs.service.h"
#include "chord.concurrent.queue.h"

using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::Status;
using grpc::StatusCode;

using chord::fs::PutResponse;
using chord::fs::PutRequest;
using chord::fs::GetResponse;
using chord::fs::GetRequest;
using chord::fs::TakeRequest;
using chord::fs::TakeResponse;


using namespace std;
using namespace chord;
using namespace chord::common;

namespace chord {
namespace fs {

Service::Service(Context &context, ChordFacade* chord)
    : context{context},
      chord{chord},
      metadata_mgr{make_unique<MetadataManager>(context)},
      make_client {[this]{
        return chord::fs::Client(this->context, this->chord);
      }},
      logger{log::get_or_create(logger_name)} { }

Service::Service(Context &context, ChordFacade* chord, IMetadataManager* metadata_mgr)
    : context{context},
      chord{chord},
      metadata_mgr{metadata_mgr},
      make_client {[this]{
        return chord::fs::Client(this->context, this->chord);
      }},
      logger{log::get_or_create(logger_name)} { }

Status Service::handle_meta_del(const MetaRequest *req) {
  const auto uri = uri::from(req->uri());
  auto metadata = MetadataBuilder::from(req);
  metadata_mgr->del(uri, metadata);
  //forward 
  auto repl_metadata_map = MetadataBuilder::group_by_replication(metadata);
  for(auto& [replication, meta] : repl_metadata_map) {
    Replication repl{replication.index, replication.count};
    if(!++repl) continue;

    //update replication
    for(auto it = meta.begin(); it != meta.end(); ++it) {
      auto node = meta.extract(it);
      if(!node) continue;
      node.value().replication = repl;
      meta.insert(std::move(node));
    }

    const auto node = make_node(chord->successor(context.uuid()));
    logger->info("Forwarding meta DEL to node {}\n{}", node, metadata);
    const auto status = make_client().meta(node, uri, Client::Action::DEL, meta);
    if(!status.ok()) {
      logger->warn("Failed to delete {} ({}) from {}", uri, repl, node);
    }
  }
  return Status::OK;
}

Status Service::handle_meta_add(const MetaRequest *req) {
  const auto uri = uri::from(req->uri());
  auto metadata = MetadataBuilder::from(req);

  const auto added = metadata_mgr->add(uri, metadata);

  auto max_repl = std::max_element(metadata.begin(), metadata.end(), [&](const auto& l, const auto &r){ 
      if( !l.replication || !r.replication) return false;
      return  l.replication->count < r.replication->count;
  })->replication;

  // update the parent (first node triggers replication)
  if(added && max_repl && max_repl->index == 0) {
    const auto parent = uri::builder{uri.scheme(), uri.path().parent_path()}.build();
    const Metadata metadata{uri.path().filename(), "", "", perms::none, fs::type::directory, {}, max_repl};
    std::set<Metadata> m = {metadata};
    make_client().meta(parent, Client::Action::ADD, m);
  }

  // handle replication
  {
    std::set<Metadata> new_metadata;
    // remove metadata
    std::copy_if(metadata.begin(), metadata.end(), std::inserter(new_metadata, new_metadata.begin()), [&](const Metadata& meta) {
        if(!meta.replication) return false;
        return (meta.replication->index+1 < meta.replication->count);
    });
    //update replication
    for(auto it = new_metadata.begin(); it != new_metadata.end(); ++it) {
      auto node = new_metadata.extract(it);
      if(!node) continue;
      node.value().replication.value()++;
      new_metadata.insert(std::move(node));
    }

    if(!new_metadata.empty()) {
      const auto node = make_node(chord->successor(context.uuid()));
      const auto status = make_client().meta(node, uri, Client::Action::ADD, new_metadata);

      if(!status.ok()) {
        logger->warn("Failed to add {} ({}) to {}", uri, *max_repl, node);
      }
    }
  }

  return Status::OK;
}

Status Service::meta(ServerContext *serverContext, const MetaRequest *req, MetaResponse *res) {
  (void)serverContext;

  const auto uri = uri::from(req->uri());

  try {
    switch (req->action()) {
      case ADD:
        return handle_meta_add(req);
      case DEL:
        return handle_meta_del(req);
      case DIR:
        set<Metadata> meta = metadata_mgr->get(uri);
        MetadataBuilder::addMetadata(meta, *res);
        break;
    }
  } catch(const chord::exception& e) {
    logger->error("failed in meta for uri {}, reason: {}", uri, e.what());
    return Status{StatusCode::NOT_FOUND, e.what()};
  }

  return Status::OK;
}

Status Service::put(ServerContext *serverContext, ServerReader<PutRequest> *reader, PutResponse *response) {
  (void)serverContext;
  (void)response;
  PutRequest req;

  path data = context.data_directory;
  if (!file::is_directory(data)) {
    file::create_directories(data);
  }

  Replication repl;
  // open
  if (reader->Read(&req)) {

    repl = {req.replication_idx(), req.replication_cnt()};

    const auto uri = uri::from(req.uri());

    data /= uri.path().parent_path();

    if (!file::exists(data)) {
      logger->trace("[put] creating directories for {}", data);
      file::create_directories(data);
    }

    data /= uri.path().filename();
    logger->trace("trying to put {}", data);

    try {
      ofstream file;
      file.exceptions(ifstream::failbit | ifstream::badbit);
      file.open(data, fstream::binary);

      // write
      do {
        file.write(req.data().data(), static_cast<std::streamsize>(req.size()));
      } while (reader->Read(&req));

    } catch (const ios_base::failure &error) {
      logger->error("failed to open file {}, reason: {}", data, error.what());
      return Status::CANCELLED;
    }
  }

  //TODO move this to facade fs_client->meta(..., ADD, ...)
  const auto uri = uri::from(req.uri());

  // add local metadata
  auto meta = MetadataBuilder::for_path(context, uri.path(), repl);
  metadata_mgr->add(uri, meta);

  // trigger recursive metadata replication for parent
  if(repl.index == 0) {
    const auto parent_uri = uri::builder{uri.scheme(), uri.path().parent_path()}.build();
    make_client().meta(parent_uri, Client::Action::ADD, meta);
  }

  // handle (recursive) file-replication
  if(++repl) {

    const chord::uuid uuid{req.id()};
    // next
    const auto next = make_node(chord->successor(context.uuid()));
    if(uuid.between(context.uuid(), next.uuid)) {
      // TODO rollback all puts?
      const auto msg = "failed to store replication " + repl.string() + " : detected cycle.";
      logger->warn(msg);
      return {StatusCode::ABORTED, msg};
    }

    path data = context.data_directory;
    data /= uri.path().parent_path();
    data /= uri.path().filename();

    std::ifstream file;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    file.open(data, std::fstream::binary);
    //TODO rollback on status ABORTED?
    make_client().put(next, uuid, uri, file, repl);
  }
  return Status::OK;
}

Status Service::del(const chord::uri& uri) {
  const auto data = context.data_directory / uri.path();

  //TODO look on some other node (e.g. the successor that this node initially joined)
  //     maybe some other node became responsible for that file recently
  if(!file::exists(data)) {
    return Status{StatusCode::NOT_FOUND, "not found"};
  }

  // remove local file
  file::remove(data);
  // remove local metadata
  const auto deleted_uris = metadata_mgr->del(uri);

  const auto parent_path = uri.path().parent_path();
  const chord::uri parent_uri{uri.scheme(), parent_path};
  auto repl_meta_map = MetadataBuilder::group_by_replication(deleted_uris);
  const auto next = make_node(chord->successor(context.uuid()));

  for(auto& [repl, metadata_set] : repl_meta_map) {
    const auto statusDel = make_client().del(uri, next);
    if(!statusDel.ok()) {
      logger->warn("Failed to delete replication for uri: {}", uri);
    }
    // update parent - if exists
    if(!parent_path.empty()) {
      const auto statusMeta = make_client().meta(next, parent_uri, Client::Action::DEL, metadata_set);
      if(!statusMeta.ok()) {
        logger->warn("Failed to delete metadata replication for uri: {}", uri);
      }
    }
  }
  return Status::OK;
}

Status Service::del(grpc::ServerContext *serverContext, const chord::fs::DelRequest *req,
           chord::fs::DelResponse *res) {
  (void)res;
  (void)serverContext;

  const auto uri = chord::uri::from(req->uri());
  auto data = context.data_directory / uri.path();

  if (!del(uri).ok()) {
    //TODO handle error
    return Status::CANCELLED;
  }

  //remove empty parent directories
  //TODO check if root
  auto parent = uri.path();
  while(!parent.empty()) {
    // directory up
    parent = parent.parent_path();
    const auto data = context.data_directory / parent;

    if(!file::is_empty(data)) break;
    if(data.canonical() <= context.data_directory.canonical()) break;

    const chord::uri parent_uri{uri.scheme(), parent};
    const auto status = del(parent_uri);
    if(!status.ok()) {
      logger->error("failed to delete empty folder {}", parent_uri);
    }
  }

  return Status::OK;
}

Status Service::get_from_reference(const chord::uri& uri) {
  const path data = context.data_directory / uri.path();
  const auto metadata_set = metadata_mgr->get(uri);

  for(Metadata m:metadata_set) {
    if(m.name != uri.path().filename().string() || !m.node_ref) 
      continue;

    if (!file::is_directory(data)) {
      logger->trace("[get] creating directories for {}", data);
      file::create_directories(data.parent_path());
    }

    try {
      ofstream ofile;
      ofile.exceptions(ofstream::failbit | ofstream::badbit);
      ofile.open(data, fstream::binary);
      const auto status = make_client().get(uri, m.node_ref.value(), ofile);
      if(!status.ok()) return status;
      return make_client().del(uri, m.node_ref.value());
    } catch (const ios_base::failure &error) {
      logger->error("failed to open file {}, reason: {}", data, error.what());
      return Status::CANCELLED;
    }
  }
  return Status::CANCELLED;
}

Status Service::get(ServerContext *serverContext, const GetRequest *req, grpc::ServerWriter<GetResponse> *writer) {
  (void)serverContext;
  ifstream file;
  file.exceptions(ifstream::failbit | ifstream::badbit);

  path data = context.data_directory;
  if (!file::is_directory(data)) {
    logger->trace("[get] creating directories for {}", data);
    file::create_directories(data);
  }

  const auto uri = chord::uri::from(req->uri());

  data /= uri.path();
  //try to lookup reference node if found in metadata
  if (!file::exists(data)) {
    logger->debug("file does not exist, trying to restore from metadata...");
    const auto status = get_from_reference(uri);
    if(!status.ok()) return status;
  } else if (!file::is_regular_file(data)) {
    logger->error("requested file is not a regular file - aborting.");
    return Status::CANCELLED;
  }

  logger->trace("trying to get {}", data);
  try {
    file.open(data, fstream::binary);
  } catch (const ios_base::failure &error) {
    logger->error("failed to open file {}, reason: {}", data, error.what());
    return Status::CANCELLED;
  }

  //TODO make configurable (see chord.client)
  constexpr size_t len = 512*1024; // 512k
  char buffer[len];
  size_t offset = 0,
         read = 0;
  do {
    read = static_cast<size_t>(file.readsome(buffer, len));
    if (read == 0) break;

    GetResponse res;
    //TODO validate hashes
    res.set_id(req->id());
    res.set_data(buffer, read);
    res.set_offset(offset);
    res.set_size(read);
    //TODO write implicit string conversion
    //res.set_uri(uri);
    offset += read;

    if (!writer->Write(res)) {
      throw__exception("broken stream.");
    }

  } while (read > 0);

  return Status::OK;
}


Status Service::take(ServerContext *serverContext,
                     const TakeRequest *req,
                     ServerWriter<TakeResponse> *writer) {
  (void)serverContext;
  logger->info("in chord.fs.service - take.......");

  TakeResponse res;
  const auto from = uuid_t{req->from()};
  const auto to = uuid_t{req->to()};
  const auto responses = produce_take_response(from, to);

  for(const auto& res:responses) {
    if (!writer->Write(res)) {
      throw__exception("broken stream.");
    }
  }
  return Status::OK;
}

} //namespace fs
} //namespace chord
