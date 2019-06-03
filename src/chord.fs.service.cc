#include <iostream>
#include <fstream>
#include <thread>
#include <experimental/filesystem>

#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include "chord.log.h"
#include "chord.uri.h"
#include "chord.utils.h"
#include "chord.file.h"
#include "chord.context.h"
#include "chord_fs.grpc.pb.h"
#include "chord.fs.context.metadata.h"
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


using namespace std;
using namespace chord;

namespace chord {
namespace fs {

Service::Service(Context &context, ChordFacade* chord)
    : context{context},
      chord{chord},
      metadata_mgr{make_unique<MetadataManager>(context)},
      make_client {[this]{
        return chord::fs::Client(this->context, this->chord);
      }},
      logger{context.logging.factory().get_or_create(logger_name)} { }

Service::Service(Context &context, ChordFacade* chord, IMetadataManager* metadata_mgr)
    : context{context},
      chord{chord},
      metadata_mgr{metadata_mgr},
      make_client {[this]{
        return chord::fs::Client(this->context, this->chord);
      }},
      logger{context.logging.factory().get_or_create(logger_name)} { }

Status Service::handle_meta_dir(const MetaRequest *req, MetaResponse *res) {
  const auto uri = uri::from(req->uri());
  if(!metadata_mgr->exists(uri)) {
    return Status{StatusCode::NOT_FOUND, "not found: " + to_string(uri)};
  }
  set<Metadata> meta = metadata_mgr->get(uri);
  MetadataBuilder::addMetadata(meta, *res);
  return Status::OK;
}

Status Service::handle_meta_del(const MetaRequest *req) {
  const auto uri = uri::from(req->uri());
  auto metadata = MetadataBuilder::from(req);
  auto deleted_metadata = metadata_mgr->del(uri, metadata);

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
        ++(*repl);
      }
      deleted_metadata.insert(std::move(node));
      if(!repl)
        deleted_metadata.erase(it);
    }
  }

  if(!deleted_metadata.empty()) {
    const auto node = chord->successor();
    const auto status = make_client().meta(node, uri, Client::Action::DEL, deleted_metadata);
  }

  return Status::OK;
}

Status Service::handle_meta_add(const MetaRequest *req) {
  const auto uri = uri::from(req->uri());
  auto metadata = MetadataBuilder::from(req);

  const auto added = metadata_mgr->add(uri, metadata);

  auto max_repl = max_replication(metadata);
  // update the parent (first node triggers replication)
  if(added && max_repl && max_repl->index == 0 && !uri.path().parent_path().empty()) {
    const auto parent = uri::builder{uri.scheme(), uri.path().parent_path()}.build();
    {
      auto meta_dir = create_directory(metadata);
      meta_dir.name = uri.path().filename();
      std::set<Metadata> m = {meta_dir};
      make_client().meta(parent, Client::Action::ADD, m);
    }
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
      const auto node = chord->successor();
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

  try {
    switch (req->action()) {
      case ADD:
        return handle_meta_add(req);
      case DEL:
        return handle_meta_del(req);
      case DIR:
        return handle_meta_dir(req, res);
    }
  } catch(const chord::exception& e) {
    const auto uri = uri::from(req->uri());
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

  Replication repl = ContextMetadata::replication_from(serverContext);
  // open
  if (reader->Read(&req)) {

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
    const auto parent_uri = chord::uri{uri.scheme(), uri.path().parent_path()};
    // add directory "." to metadata
    meta.insert(create_directory(meta));
    make_client().meta(parent_uri, Client::Action::ADD, meta);
  }

  // handle (recursive) file-replication
  if(++repl) {

    const auto uuid = chord::crypto::sha256(uri);
    // next
    const auto next = chord->successor();
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
    make_client().put(next, uri, file, repl);
  }
  return Status::OK;
}

Status Service::handle_del_file(const chord::fs::DelRequest *req) {
  const auto uri = chord::uri::from(req->uri());
  auto data = context.data_directory / uri.path();

  auto deleted_metadata = metadata_mgr->del(uri);
  const auto iter = std::find_if(deleted_metadata.begin(), deleted_metadata.end(), [&](const Metadata& m) { return m.name == uri.path().filename();});
  // handle shallow copy
  if(iter->node_ref) {
    make_client().del(*iter->node_ref, req);
  }
  // handle local file
  if(file::exists(data)) {
    file::remove(data);
  }

  // beg: handle replica
  const auto node = chord->successor();
  auto max_repl = max_replication(deleted_metadata);
  const bool initial_delete = chord->successor(crypto::sha256(req->uri())) == context.node();
  const chord::uri parent_uri{uri.scheme(), uri.path().parent_path()};

  if(max_repl) {
    if(max_repl->index+1 < max_repl->count) {
      //TODO handle status
      make_client().del(node, req);
    }
    if(initial_delete) {
      make_client().meta(parent_uri, Client::Action::DEL, deleted_metadata);
    }
  }
  // end: handle replica

  // beg: check whether directory is empty now
  if(initial_delete) {
    const auto parent_path = data.parent_path();
    if(file::is_empty(parent_path)) {
      return make_client().del(parent_uri);
    }
  }
  // end: check whether directory is empty now

  // handle metadata
  return Status::OK;
}

Status Service::handle_del_dir(const chord::fs::DelRequest *req) {
  const auto uri = chord::uri::from(req->uri());
  auto data = context.data_directory / uri.path();

  if(file::exists(data) && file::is_directory(data) && !file::is_empty(data) && !req->recursive()) {
    return Status{StatusCode::FAILED_PRECONDITION, fmt::format("failed to delete {}: Directory not empty", to_string(uri))};
  }

  auto metadata = metadata_mgr->get(uri);

  for(const auto m:metadata) {
    if(m.name == ".") continue;
    const auto sub_uri = uri::builder{uri.scheme(), uri.path() / path{m.name}}.build();
    const auto status = make_client().del(sub_uri, req->recursive());
    if(!status.ok()) {
      logger->error("Failed to delete directory: {} {}", status.error_message(), status.error_details());
      return status;
    }
  }

  if(file::exists(data) && file::is_empty(data)) {

    const bool initial_delete = chord->successor(crypto::sha256(req->uri())) == context.node();
    {
      // initial node triggers parent metadata deletion
      if(initial_delete) {
        const chord::uri parent_uri{uri.scheme(), uri.path().parent_path()};
        Metadata deleted_dir = Metadata();
        deleted_dir.name = uri.path().filename();
        std::set<Metadata> metadata{deleted_dir};
        make_client().meta(parent_uri, Client::Action::DEL, metadata);
      }
    }

    file::remove(data);
    metadata_mgr->del(uri);

    const auto node = chord->successor();
    // just forward the request and exit on status == NotFound
    make_client().del(node, req);
  }

  return Status::OK;
}

Status Service::del(grpc::ServerContext *serverContext,
                    const chord::fs::DelRequest *req,
                    chord::fs::DelResponse *res) {
  (void)res;
  (void)serverContext;

  const auto uri = chord::uri::from(req->uri());
  auto data = context.data_directory / uri.path();

  //FIXME this wont work for shallow-copied files (metadata-only)
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
    return handle_del_dir(req);
  } else {
    return handle_del_file(req);
  }

  return Status::OK;
}

Status Service::get_from_reference_or_replication(const chord::uri& uri) {
  const path data = context.data_directory / uri.path();
  const auto metadata_set = metadata_mgr->get(uri);

  for(Metadata m:metadata_set) {
    if(m.name != uri.path().filename().string() || !m.node_ref) 
      continue;

    if (!file::is_directory(data)) {
      logger->trace("[get] creating directories for {}", data);
      file::create_directories(data.parent_path());
    }

    grpc::Status status = Status::CANCELLED;

    // try to get by node reference
    if(m.node_ref) {
      try {
        status = make_client().get(uri, m.node_ref.value(), data);
        if(!status.ok()) {
          logger->warn("failed to get from referenced node - trying to get replication.");
        }
        return make_client().del(*m.node_ref, uri);
      } catch (const ios_base::failure &error) {
        logger->error("failed to open file {}, reason: {}", data, error.what());
        return Status::CANCELLED;
      }
    }
    // try to get replication
    if(m.replication && *m.replication) {
      try {
        const auto successor = chord->successor();
        status = make_client().get(uri, successor, data);
        if(!status.ok()) {
          logger->warn("failed to get from referenced node - trying to get replication.");
        }
        return make_client().del(*m.node_ref, uri);
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
  //try to lookup reference node if found in metadata
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
