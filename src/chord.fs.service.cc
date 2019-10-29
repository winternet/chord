#include "chord.fs.service.h"

#include <fstream>
#include <cstddef>
#include <algorithm>
#include <set>
#include <string>
#include <utility>

#include <grpcpp/impl/codegen/status_code_enum.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/sync_stream.h>

#include "chord.context.h"
#include "chord.crypto.h"
#include "chord.exception.h"
#include "chord.facade.h"
#include "chord.file.h"
#include "chord.fs.client.h"
#include "chord.fs.context.metadata.h"
#include "chord.fs.metadata.builder.h"
#include "chord.fs.metadata.h"
#include "chord.fs.metadata.manager.h"
#include "chord.fs.replication.h"
#include "chord.fs.type.h"
#include "chord.log.factory.h"
#include "chord.log.h"
#include "chord.node.h"
#include "chord.path.h"
#include "chord.uri.h"
#include "chord.uuid.h"

using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::Status;
using grpc::StatusCode;

using chord::fs::PutResponse;
using chord::fs::PutRequest;
using chord::fs::GetResponse;
using chord::fs::GetRequest;


using namespace std;
using namespace chord;

namespace chord {
namespace fs {

Service::Service(Context &context, ChordFacade* chord, IMetadataManager* metadata_mgr)
    : context{context},
      chord{chord},
      metadata_mgr{metadata_mgr},
      make_client {[this]{
        return chord::fs::Client(this->context, this->chord, this->metadata_mgr);
      }},
      logger{context.logging.factory().get_or_create(logger_name)} { }

Status Service::is_valid(ServerContext* serverContext) {
  const auto src = ContextMetadata::src_from(serverContext);
  const auto is_rebalance = ContextMetadata::rebalance_from(serverContext);
  if(!is_rebalance && src == context.uuid()) {
    logger->error("Invalid request: received request from self");
    return Status(StatusCode::ABORTED, "received request from self - aborting.");
  }
  return Status::OK;
}

Status Service::handle_meta_dir(ServerContext *serverContext, const MetaRequest *req, MetaResponse *res) {
  const auto uri = uri::from(req->uri());
  if(!metadata_mgr->exists(uri)) {
    return Status{StatusCode::NOT_FOUND, "not found: " + to_string(uri)};
  }
  set<Metadata> meta = metadata_mgr->get(uri);
  MetadataBuilder::addMetadata(meta, *res);
  return Status::OK;
}

Status Service::handle_meta_del(ServerContext *serverContext, const MetaRequest *req) {
  const auto uri = uri::from(req->uri());
  auto metadata = MetadataBuilder::from(req);
  auto deleted_metadata = metadata_mgr->del(uri, metadata);
  const auto src = ContextMetadata::src_from(serverContext);
  /**
   * increase and check replication
   * remove metadata or propagate deletion
   */
  {
    for(auto it = deleted_metadata.begin(); it != deleted_metadata.end(); ++it) {
      auto node = deleted_metadata.extract(it);
      if(!node) continue;
      auto& repl = node.value().replication;
      if(repl) {
        ++repl;
      }
      deleted_metadata.insert(std::move(node));
      if(!repl)
        deleted_metadata.erase(it);
    }
  }

  if(!deleted_metadata.empty()) {
    const auto node = chord->successor();
    client::options options;
    options.source = src;
    const auto status = make_client().meta(node, uri, Client::Action::DEL, deleted_metadata, options);
  }

  return Status::OK;
}

Status Service::handle_meta_add(ServerContext *serverContext, const MetaRequest *req) {
  const auto uri = uri::from(req->uri());
  auto metadata = MetadataBuilder::from(req);

  const auto added = metadata_mgr->add(uri, metadata);
  const auto src = ContextMetadata::src_from(serverContext);

  auto max_repl = max_replication(metadata);
  // update the parent (first node triggers replication)
  if(added && max_repl.index == 0 && !uri.path().parent_path().empty()) {
    const auto parent = uri::builder{uri.scheme(), uri.path().parent_path()}.build();
    {
      auto meta_dir = create_directory(metadata);
      meta_dir.name = uri.path().filename();
      std::set<Metadata> m = {meta_dir};
      client::options options;
      options.source = src;
      make_client().meta(parent, Client::Action::ADD, m, options);
    }
  }

  // handle replication
  {
    std::set<Metadata> new_metadata;
    // remove metadata
    std::copy_if(metadata.begin(), metadata.end(), std::inserter(new_metadata, new_metadata.begin()), [&](const Metadata& meta) {
        if(!meta.replication) return false;
        return meta.replication.has_next();
    });
    //update replication
    for(auto it = new_metadata.begin(); it != new_metadata.end(); ++it) {
      auto node = new_metadata.extract(it);
      if(!node) continue;
      node.value().replication++;
      new_metadata.insert(std::move(node));
    }

    if(!new_metadata.empty()) {
      const auto node = chord->successor();
      client::options options;
      options.source = src;
      const auto status = make_client().meta(node, uri, Client::Action::ADD, new_metadata, options);

      if(!status.ok()) {
        logger->warn("Failed to add {} ({}) to {}", uri, max_repl, node);
      }
    }
  }

  return Status::OK;
}

Status Service::meta(ServerContext *serverContext, const MetaRequest *req, MetaResponse *res) {
  (void)serverContext;

  // in case controller service queries...
  //const auto status = is_valid(serverContext);
  //if(!status.ok()) {
  //  return status;
  //}

  try {
    switch (req->action()) {
      case ADD:
        return handle_meta_add(serverContext, req);
      case DEL:
        return handle_meta_del(serverContext, req);
      case DIR:
        return handle_meta_dir(serverContext, req, res);
    }
  } catch(const chord::exception& e) {
    const auto uri = uri::from(req->uri());
    logger->error("failed in meta for uri {}, reason: {}", uri, e.what());
    return Status{StatusCode::NOT_FOUND, e.what()};
  }

  return Status::OK;
}

bool Service::file_hashes_equal(grpc::ServerContext* serverContext, grpc::ServerReader<PutRequest>* reader) {
  const auto hash = ContextMetadata::file_hash_from(serverContext);
  const auto uri = ContextMetadata::uri_from(serverContext);

    auto hashes_equal = false;
  // TODO: handle receive an update
  if(hash && metadata_mgr->exists(uri)) {
    const auto metadata_set = metadata_mgr->get(uri);

    if(fs::is_regular_file(metadata_set)) {
      const auto metadata = *metadata_set.begin();
      const auto local_hash = metadata.file_hash;
      hashes_equal = (local_hash && local_hash == hash);
      ContextMetadata::set_file_hash_equal(serverContext, hashes_equal);
    }
  }
  reader->SendInitialMetadata();
  return hashes_equal;
}

Status Service::put(ServerContext *serverContext, ServerReader<PutRequest> *reader, PutResponse *response) {
  (void)response;
  PutRequest req;

  const auto status = is_valid(serverContext);
  if(!status.ok()) {
    return status;
  }

  path data = context.data_directory;
  if (!file::is_directory(data)) {
    file::create_directories(data);
  }

  client::options options = ContextMetadata::from(serverContext);
  const auto uri = ContextMetadata::uri_from(serverContext);
  bool del_needed = options.rebalance && !options.replication.has_next();

  // open - if needed (file hashes do not equal)
  if (!file_hashes_equal(serverContext, reader) && reader->Read(&req)) {

    data /= uri.path().parent_path();

    if (!file::exists(data)) {
      logger->trace("[put] creating directories for {}", data);
      file::create_directories(data);
    }

    data /= uri.path().filename();
    logger->trace("trying to put {}", data);

    crypto::sha256_hasher hasher;
    try {
      ofstream file;
      file.exceptions(ifstream::failbit | ifstream::badbit);
      file.open(data, fstream::binary);

      // write
      do {
        const auto data = req.data().data();
        const auto len = req.size();
        hasher(data, len);
        file.write(data, static_cast<std::streamsize>(len));
      } while (reader->Read(&req));

    } catch (const ios_base::failure &error) {
      logger->error("failed to open file {}, reason: {}", data, error.what());
      return Status::CANCELLED;
    }
  }

  // add local metadata
  auto meta = MetadataBuilder::for_path(context, uri.path(), options.replication);
  metadata_mgr->add(uri, meta);

  // trigger recursive metadata replication for parent
  if(options.replication.index == 0) {
    const auto parent_uri = chord::uri{uri.scheme(), uri.path().parent_path()};
    // add directory "." to metadata
    meta.insert(create_directory(meta));
    make_client().meta(parent_uri, Client::Action::ADD, meta, options);
  }

  // handle (recursive) file-replication
  if(++options.replication) {

    //const auto uuid = chord::crypto::sha256(uri);
    // next
    const auto next = chord->successor();
    // TODO use some kind of call-uuid + logging
    //if(uuid.between(context.uuid(), next.uuid)) {
    //  // TODO rollback all puts?
    //  const auto msg = "failed to store replication " + repl.string() + " : detected cycle.";
    //  logger->warn(msg);
    //  return {StatusCode::ABORTED, msg};
    //}

    path data = context.data_directory;
    data /= uri.path().parent_path();
    data /= uri.path().filename();

    std::ifstream file;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    file.open(data, std::fstream::binary);
    //TODO rollback on status ABORTED?
    //options.replication = repl;
    make_client().put(next, uri, file, options);
  }
  if(del_needed) {
    const auto hash = chord::crypto::sha256(uri);
    const auto next = chord->successor();
    // skip deletion if reach begin
    // TODO: use ContextMetadata src (!)
    if(next != chord->successor(hash)) {
      make_client().del(next, uri, false, options);
    } else {
      logger->trace("would delete from begin, skipping.");
    }
  }
  return Status::OK;
}

Status Service::handle_del_file(ServerContext *serverContext, const chord::fs::DelRequest *req) {
  const auto uri = chord::uri::from(req->uri());
  auto data = context.data_directory / uri.path();

  const auto src = ContextMetadata::src_from(serverContext);
  auto deleted_metadata = metadata_mgr->del(uri);
  const auto iter = std::find_if(deleted_metadata.begin(), deleted_metadata.end(), [&](const Metadata& m) { return m.name == uri.path().filename();});

  client::options options;
  options.source = src;
  // handle shallow copy
  if(iter != deleted_metadata.end() && iter->node_ref && *iter->node_ref != context.node()) {
    make_client().del(*iter->node_ref, req, options);
  }
  // handle local file
  if(file::exists(data)) {
    file::remove(data);
  }

  // beg: handle replica
  const auto max_repl = max_replication(deleted_metadata);
  const bool initial_delete = chord->successor(crypto::sha256(req->uri())) == context.node();
  const chord::uri parent_uri{uri.scheme(), uri.path().parent_path()};

  if(max_repl && max_repl.has_next()) {
    //TODO handle status
    const auto node = chord->successor();
    make_client().del(node, req, options);
  }
  // end: handle replica
 

  // beg: check whether directory is empty now -> delete metadata
  const auto parent_path = data.parent_path();
  const auto dir_empty = file::exists(parent_path) && file::is_empty(parent_path);
  if(initial_delete && dir_empty) {
    return make_client().del(parent_uri, false, options);
  } else if(initial_delete && !parent_uri.path().empty()) {
    // beg: update parent path
    make_client().meta(parent_uri, Client::Action::DEL, deleted_metadata, options);
    //TODO handle return status
    // end: update parent path
  } 
  // end: check whether directory is empty now
  
  // beg: handle empty directories
  if(dir_empty) {
    // node joined and the files were shifted to the other node
    file::remove(parent_path);
  }
  // end: handle empty directories

  // handle metadata
  return Status::OK;
}

Status Service::handle_del_dir(ServerContext *serverContext, const chord::fs::DelRequest *req) {
  const auto uri = chord::uri::from(req->uri());
  auto data = context.data_directory / uri.path();

  if(!file::exists(data)) {
    return Status(StatusCode::NOT_FOUND, fmt::format("failed to delete {}: directory not found.", to_string(uri)));
  }

  const auto is_empty = file::is_empty(data);

  if(file::is_directory(data) && !is_empty && !req->recursive()) {
    return Status{StatusCode::FAILED_PRECONDITION, fmt::format("failed to delete {}: directory not empty - missing recursive flag?", to_string(uri))};
  }

  const auto src = ContextMetadata::src_from(serverContext);
  client::options options;
  options.source = src;

  // handle recursive delete
  if(!is_empty && req->recursive()) {
    // remove local metadata of directory
    const auto metadata = metadata_mgr->get(uri);

    // handle possibly remote metadata of sub-uris
    for(const auto m:metadata) {
      if(m.name == ".") continue;
      const auto sub_uri = chord::uri(uri.scheme(), uri.path() / m.name);
      const auto status = make_client().del(sub_uri, req->recursive(), options);
      if(!status.ok()) {
        logger->error("failed to delete directory: {} {}", status.error_message(), status.error_details());
        return status;
      }
    }
  }

  // after all contents have been deleted - check emptyness again
  if(!file::is_empty(data)) {
    return Status(StatusCode::ABORTED, fmt::format("failed to delete {}: directory not empty", to_string(uri)));
  }

  // beg: handle local
  file::remove(data);
  const auto deleted_metadata = metadata_mgr->del(uri);
  // end: handle local

  // beg: handle parent
  const bool initial_delete = chord->successor(crypto::sha256(req->uri())) == context.node();
  {
    const auto parent_path = uri.path().parent_path();
    // initial node triggers parent metadata deletion
    if(initial_delete && !parent_path.empty()) {
      Metadata deleted_dir = Metadata();
      deleted_dir.name = uri.path().filename();
      std::set<Metadata> metadata{deleted_dir};
      make_client().meta(chord::uri(uri.scheme(), parent_path), Client::Action::DEL, metadata, options);
    }
  }
  // end: handle parent

  // beg: handle replica
  auto max_repl = max_replication(deleted_metadata);
  if(++max_repl) {
    const auto node = chord->successor();
    make_client().del(node, req, options);
  }
  // end: handle replica

  return Status::OK;
}

Status Service::del(grpc::ServerContext *serverContext,
                    const chord::fs::DelRequest *req,
                    chord::fs::DelResponse *res) {
  (void)res;
  (void)serverContext;

  const auto status = is_valid(serverContext);
  if(!status.ok()) {
    return status;
  }

  const auto uri = chord::uri::from(req->uri());
  auto data = context.data_directory / uri.path();

  // we receive a delete from a former-shallow-copy node
  //if(file::exists(data) && file::is_regular_file(data) && !metadata_mgr->exists(uri)) {
  //  logger->trace("handling delete request from shallow-copy node ({}).", uri);
  //  const auto success = file::remove(data);
  //  return success ? Status::OK : Status::CANCELLED;
  //}

  //TODO look on some other node (e.g. the successor that this node initially joined)
  //     maybe some other node became responsible for that file recently
  const auto metadata_set = metadata_mgr->get(uri);
  const auto element = std::find_if(metadata_set.begin(), metadata_set.end(), [&](const Metadata& metadata) {
      return metadata.name == uri.path().filename() || (metadata.name == "." && metadata.file_type == type::directory);
  });

  if(element == metadata_set.end()) {
    return Status{StatusCode::NOT_FOUND, "not found"};
  }

  if(element->file_type == type::directory) {
    return handle_del_dir(serverContext, req);
  } else {
    return handle_del_file(serverContext, req);
  }

  return Status::OK;
}

Status Service::get_from_reference_or_replication(const chord::uri& uri) {
  const path data = context.data_directory / uri.path();
  const auto metadata_set = metadata_mgr->get(uri);

  for(Metadata m:metadata_set) {
    // this doesnt make sense...?
    //if(m.name != uri.path().filename().string() || !m.node_ref) 
    //  continue;

    if (!file::is_directory(data)) {
      logger->trace("[get] creating directories for {}", data);
      file::create_directories(data.parent_path());
    }

    auto status = Status::CANCELLED;

    // try to get by node reference
    if(m.node_ref) {
      try {
        const auto node_ref = *m.node_ref;
        status = make_client().get(uri, node_ref, data);
        if(!status.ok()) {
          logger->warn("failed to get from referenced node - trying to get replication.");
        } else {
          logger->trace("successfully received file - reset node ref.");
          m.node_ref = {};
          m.file_hash = crypto::sha256(data);
          std::set<Metadata> metadata = {m};
          metadata_mgr->add(uri, metadata);
          //TODO: check whether we need src
          //return make_client().del(node_ref, uri, false, src);
          return make_client().del(node_ref, uri);
        }
      } catch (const ios_base::failure &error) {
        logger->error("failed to open file {}, reason: {}", data, error.what());
        return Status::CANCELLED;
      }
    }
    // try to get replication
    if(m.replication) {
      try {
        const auto successor = chord->successor();
        status = make_client().get(uri, successor, data);
        if(!status.ok()) {
          logger->warn("failed to get from referenced node - trying to get replication.");
        }
        return status;
      } catch (const ios_base::failure &error) {
        logger->error("failed to open file {}, reason: {}", data, error.what());
        return Status::CANCELLED;
      }
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
  // try to get file
  if (!file::exists(data)) {
    logger->debug("file does not exist, trying to restore from metadata...");
    const auto status = get_from_reference_or_replication(uri);
    if(!status.ok()) {
      return status;
    }
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

} //namespace fs
} //namespace chord
