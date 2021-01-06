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

#include "chord.fs.common.h"
#include "chord.fs.monitor.h"
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
#include "chord.fs.util.h"
#include "chord.fs.type.h"
#include "chord.log.factory.h"
#include "chord.log.h"
#include "chord.node.h"
#include "chord.path.h"
#include "chord.uri.h"
#include "chord.utils.h"
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

Service::Service(Context &context, ChordFacade* chord, IMetadataManager* metadata_mgr, Client* client, chord::fs::monitor* monitor)
    : context{context},
      chord{chord},
      client{client},
      metadata_mgr{metadata_mgr},
      monitor{monitor},
      make_client {[this]{ return this->client; }},
      logger{context.logging.factory().get_or_create(logger_name)} { }

Status Service::is_valid(ServerContext* serverContext, const RequestType req_type) {
  const auto options = ContextMetadata::from(serverContext);
  const bool src_equals_this = options.source == context.uuid();
  if(src_equals_this) {
    const auto message = "received request from self.";
    logger->info(message);
    return Status(StatusCode::ALREADY_EXISTS, message);
  }
  return Status::OK;
}

client::options Service::init_source(const client::options& o) const {
  return chord::fs::client::init_source(o, context);
}

Status Service::handle_meta_mov([[maybe_unused]] ServerContext *serverContext, const MetaRequest *req, MetaResponse *res) {
  const auto uri = uri::from(req->uri());
  const auto dst = uri::from(req->uri_dst());
  const auto options = ContextMetadata::from(serverContext);

  // check metadata exists
  if(!metadata_mgr->exists(uri)) {
    const auto message = "not found: " + to_string(uri);
    logger->info(message);
    return Status{StatusCode::NOT_FOUND, message};
  }
  // check file exists
  auto data = context.data_directory / uri.path();
  if(!chord::file::exists(data)) {
    const auto message = "not found: " + to_string(uri);
    logger->info(message);
    return Status{StatusCode::NOT_FOUND, message};
  }

  auto status = make_client()->put(dst, data, options);
  if(status.ok()) {
    return make_client()->del(uri);
  }

  return status;
}

Status Service::handle_meta_dir([[maybe_unused]] ServerContext *serverContext, const MetaRequest *req, MetaResponse *res) {
  const auto uri = uri::from(req->uri());

  if(!metadata_mgr->exists(uri)) {
    const auto message = "not found: " + to_string(uri);
    logger->info(message);
    return Status{StatusCode::NOT_FOUND, message};
  }

  const auto meta = metadata_mgr->get(uri);
  MetadataBuilder::addMetadata(meta, *res);
  return Status::OK;
}

Status Service::handle_meta_del(ServerContext *serverContext, const MetaRequest *req) {
  const auto uri = uri::from(req->uri());

  auto deleted_metadata = metadata_mgr->del(uri, MetadataBuilder::from(req));
  const auto options = ContextMetadata::from(serverContext);

  /**
   * increase and check replication
   * remove metadata or propagate deletion
   */
  std::set<Metadata> metadata = chord::fs::increase_replication_and_clean(deleted_metadata);

  if(!is_empty(metadata)) {
    const auto node = chord->successor();
    const auto status = make_client()->meta(node, uri, Client::Action::DEL, metadata, init_source(options));
  }

  /**
   * NOTE this will invoke a del_dir request on the uri since it is empty now.
   */
  {
    const auto metadata_set = metadata_mgr->get(uri);
    if(is_empty(metadata_set)) {
      make_client()->del(uri, false, clear_source(options));
    }
  }

  return Status::OK;
}

Status Service::handle_meta_add(ServerContext *serverContext, const MetaRequest *req) {
  const auto uri = uri::from(req->uri());
  auto metadata = MetadataBuilder::from(req);

  if(metadata_mgr->exists(uri) &&
    Metadata::equal_basic(metadata_mgr->get(uri), metadata)) {
    return {StatusCode::ALREADY_EXISTS, "metadata already exists."};
  }

  // regular file is not shallow copyable because it already exists (locally)
  //if(is_regular_file(metadata) 
  //    && is_shallow_copy(metadata, context) 
  //    && metadata_mgr->exists(uri)) {
  //  const auto existing = metadata_mgr->get(uri);
  //  const auto same = std::equal(metadata.begin(), metadata.end(), existing.begin(), existing.end(), [&](const auto& lhs, const auto& rhs) {return lhs.compare_basic(rhs);});
  //  if(same) {
  //    const auto message = "received request from self.";
  //    return Status{StatusCode::ALREADY_EXISTS, message};
  //  }
  //}

  const auto added = metadata_mgr->add(uri, is_mkdir(uri, metadata) ? std::set{create_directory(metadata)} : metadata);

  const auto options = ContextMetadata::from(serverContext);

  auto max_repl = max_replication(metadata);
  // update the parent (first node triggers replication)
  const auto parent_path = uri.path().parent_path();
  if(added && max_repl.index == 0 && !parent_path.empty() /*&& fs::is_directory(metadata)*/) {
    const auto parent_uri = chord::uri{uri.scheme(), parent_path};
    {
      auto meta_dir = create_directory(metadata, uri.path().filename());
      make_client()->meta(parent_uri, Client::Action::ADD, meta_dir, options);
    }
  }

  // handle replication
  {
    std::set<Metadata> new_metadata = chord::fs::increase_replication_and_clean(metadata);

    if(!new_metadata.empty()) {
      const auto node = chord->successor();
      const auto status = make_client()->meta(node, uri, Client::Action::ADD, new_metadata, init_source(options));

      if(!is_successful(status)) {
        logger->warn("[meta][add] failed to add {} ({}) to {}", uri, max_repl, node);
      }
    }
  }

  return Status::OK;
}

Status Service::meta([[maybe_unused]] ServerContext *serverContext, const MetaRequest *req, MetaResponse *res) {

  // in case controller service queries...
  const auto status = is_valid(serverContext, RequestType::META);
  if(!status.ok()) {
    return status;
  }

  try {
    switch (req->action()) {
      case ADD:
        return handle_meta_add(serverContext, req);
      case DEL:
        return handle_meta_del(serverContext, req);
      case DIR:
        return handle_meta_dir(serverContext, req, res);
      case MOV:
        return handle_meta_mov(serverContext, req, res);
    }
  } catch(const chord::exception& e) {
    const auto uri = uri::from(req->uri());
    //logger->error("failed in meta for uri {}, reason: {}", uri, e.what());
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
    }
  }
  ContextMetadata::set_file_hash_equal(serverContext, hashes_equal);
  reader->SendInitialMetadata();
  return hashes_equal;
}

Status Service::put(ServerContext *serverContext, ServerReader<PutRequest> *reader, [[maybe_unused]] PutResponse *response) {

  const auto status = is_valid(serverContext, RequestType::PUT);
  if(!status.ok()) {
    return status;
  }

  path data = context.data_directory;
  if (!file::is_directory(data)) {
    file::create_directories(data);
  }

  auto options = ContextMetadata::from(serverContext);
  const auto uri = ContextMetadata::uri_from(serverContext);
  bool del_needed = options.rebalance && !options.replication.has_next();

  // open - if needed (file hashes do not equal)
  PutRequest req;
  try {
    // create parent path
    data /= uri.path().parent_path();
    if (!file::exists(data)) {
      logger->trace("[put] creating directories for {}", data);
      file::create_directories(data);
    }
    data /= uri.path().filename();

    const auto lock = monitor::lock(monitor, {data, chord::fs::monitor::event::flag::CREATED});

    const auto file_exists = file::exists(data);

    if (!file_hashes_equal(serverContext, reader) && reader->Read(&req)) {
      logger->trace("[put] {}", data);

      crypto::sha256_hasher hasher;

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

    } else if(!file_exists) {
      // empty file was put
      chord::file::create_file(data);
    }
  } catch (const ios_base::failure &error) {
    logger->error("[put] {}, reason: {}", data, error.what());
    //TODO
    return Status::CANCELLED;
  }

  // add local metadata
  auto meta = MetadataBuilder::for_path(context, uri.path(), options.replication);
  metadata_mgr->add(uri, meta);

  // trigger recursive metadata replication for parent
  if(options.replication.index == 0) {
    const auto parent_uri = chord::uri{uri.scheme(), uri.path().parent_path()};
    // add directory "." to metadata
    meta.insert(create_directory(meta));
    make_client()->meta(parent_uri, Client::Action::ADD, meta, options);
  }

  // handle (recursive) file-replication
  if(++options.replication) {

    const auto next = chord->successor();

    path data = context.data_directory;
    data /= uri.path().parent_path();
    data /= uri.path().filename();

    std::ifstream file;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    file.open(data, std::fstream::binary);
    //TODO rollback on status ABORTED?
    make_client()->put(next, uri, file, init_source(options));
  }
  if(del_needed) {
    const auto hash = chord::crypto::sha256(uri);
    const auto next = chord->successor();
    if(next != chord->successor(hash)) {
      logger->trace("[put] deletion of file from successor needed.");
      make_client()->del(next, uri, false, init_source(options));
    }
  }
  return Status::OK;
}

Status Service::handle_del_file(ServerContext *serverContext, const chord::fs::DelRequest *req) {
  const auto uri = chord::uri::from(req->uri());
  auto data = context.data_directory / uri.path();

  const auto options = ContextMetadata::from(serverContext);
  const bool initial_delete = !options.source;

  auto deleted_metadata = metadata_mgr->del(uri);
  const auto iter = std::find_if(deleted_metadata.begin(), deleted_metadata.end(), [&](const Metadata& m) { return m.name == uri.path().filename();});

  // handle shallow copy
  if(iter != deleted_metadata.end() && iter->node_ref && *iter->node_ref != context.node()) {
    //TODO return here what about my metadata?
    make_client()->del(*iter->node_ref, req, init_source(options));
  }

  // handle local file
  if(file::exists(data)) {
    //const auto lock = monitor::lock(monitor, {data, chord::fs::monitor::event::flag::REMOVED});
    fs::util::remove(uri, context, monitor);
    //file::remove(data);
  }

  // beg: handle replica
  const auto max_repl = max_replication(deleted_metadata);

  if(max_repl && max_repl.has_next()) {
    //TODO handle status
    const auto node = chord->successor();
    if(node != context.node()) 
      make_client()->del(node, req, init_source(options));
  }
  // end: handle replica
 

    if(initial_delete) {
      const auto parent_uri = chord::uri{uri.scheme(), uri.path().parent_path()};
      make_client()->meta(parent_uri, Client::Action::DEL, deleted_metadata, clear_source(options));
    } 
  // end: check whether directory is empty now
  
  return Status::OK;
}

Status Service::handle_del_recursive(ServerContext *serverContext, const chord::fs::DelRequest *req) {

    const auto uri = chord::uri::from(req->uri());
    const auto metadata = metadata_mgr->get(uri);
    const auto options = ContextMetadata::from(serverContext);

    // handle possibly remote metadata of sub-uris
    for(const auto& m:metadata) {
      if(is_directory(m)) continue;
      const auto sub_uri = chord::uri(uri.scheme(), uri.path() / m.name);
      const auto status = make_client()->del(sub_uri, req->recursive(), clear_source(options));
      if(!is_successful(status)) {
        logger->error("[del][dir] failed to delete directory {}: {}", sub_uri, utils::to_string(status));
        return status;
      }
    }
    /**
     * all (recursive) deletions of all sub-files and sub-folders are triggered,
     * in case the last file/directory has been deleted the meta(<parent>, DEL, <last_file>)
     * will detect the empty folder and trigger the deletion of this folder using
     * meta -> del
     */
    return Status::OK;
}

Status Service::handle_del_dir(ServerContext *serverContext, const chord::fs::DelRequest *req) {
  const auto uri = chord::uri::from(req->uri());
  auto data = context.data_directory / uri.path();

  const auto exists = file::exists(data);
  const auto is_empty = exists && file::is_empty(data);

  if(exists && file::is_directory(data) && !is_empty && !req->recursive()) {
    return Status{StatusCode::FAILED_PRECONDITION, fmt::format("failed to delete {}: directory not empty - missing recursive flag?", to_string(uri))};
  }

  // if already locally deleted or ...
  if(/*!exists || */(!is_empty && req->recursive())) {
    return handle_del_recursive(serverContext, req);
  } 

  // directory was recursively emptied and handle_del_dir was called remotely
  {
    const auto options = ContextMetadata::from(serverContext);
    const bool initial_delete = !options.source;

    // beg: handle local
    // note that the directory will be cleared first if not empty;
    // in case the directory is_empty the directory will be deleted
    // implicitly in the wake of the file deletions
    //TODO check whether we need to move this into the if condition...
    auto deleted_metadata = metadata_mgr->del(uri);
    // end: handle local


    // beg: handle replica
    auto max_repl = max_replication(deleted_metadata);
    if(++max_repl) {
      const auto node = chord->successor();
      if(node != context.node())
        make_client()->del(node, req, init_source(options));
    }
    // end: handle replica

    // beg: handle local
    if(file::exists(data) && file::is_empty(data)) {
      const auto lock = monitor::lock(monitor, {data, chord::fs::monitor::event::flag::REMOVED});
      file::remove(data);
    }
    // end: handle local

    /**
    * NOTE: we must delete the data locally before we handle the parent's metadata
    * since the meta(..., DEL,... ) will issue a del(..., recursive=false) which
    * assumes that the directory is already empty.
    */
    // beg: handle parent
    {
      const auto parent_path = uri.path().parent_path();
      if(initial_delete && !parent_path.empty()) {
        auto metadata_set = std::set{MetadataBuilder::directory(uri.path())};
        make_client()->meta(chord::uri(uri.scheme(), parent_path), Client::Action::DEL, metadata_set, clear_source(options));
      }
    }
    // end: handle parent
    return Status::OK;
  }
}

Status Service::del([[maybe_unused]] grpc::ServerContext *serverContext,
                    const chord::fs::DelRequest *req,
                    [[maybe_unused]] chord::fs::DelResponse *res) {
  const auto status = is_valid(serverContext, RequestType::DEL);
  if(!status.ok()) {
    return status;
  }

  const auto uri = chord::uri::from(req->uri());
  auto data = context.data_directory / uri.path();

  // we receive a delete from a former-shallow-copy node
  if(file::exists(data) && file::is_regular_file(data) && !metadata_mgr->exists(uri)) {
    logger->trace("[del] handling shallow-copy delete for: {}", uri);
    const auto success = file::remove(data);
    return success ? Status::OK : Status::CANCELLED;
  }

  //TODO look on some other node (e.g. the successor that this node initially joined)
  //     maybe some other node became responsible for that file recently
  const auto metadata_set = metadata_mgr->get(uri);
  const auto element = std::find_if(metadata_set.begin(), metadata_set.end(), [&](const Metadata& metadata) {
      return metadata.name == uri.path().filename() || fs::is_directory(metadata);
  });

  if(element == metadata_set.end()) {
    return Status{StatusCode::NOT_FOUND, fmt::format("not found: {}", uri)};
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
        status = make_client()->get(uri, node_ref, data);
        if(!status.ok()) {
          logger->warn("[get] failed to get from referenced node - trying to get replication.");
        } else {
          logger->trace("[get] successfully received file - reset node ref.");
          m.node_ref = {};
          m.file_hash = crypto::sha256(data);
          std::set<Metadata> metadata = {m};
          metadata_mgr->add(uri, metadata);
          //TODO: check whether we need src
          //return make_client()->del(node_ref, uri, false, src);
          return make_client()->del(node_ref, uri);
        }
      } catch (const ios_base::failure &error) {
        logger->error("[get] failed to open file {}, reason: {}", data, error.what());
        return Status::CANCELLED;
      }
    }
    // try to get replication
    if(m.replication) {
      try {
        const auto successor = chord->successor();
        status = make_client()->get(uri, successor, data);
        if(!status.ok()) {
          logger->warn("[get] failed to get from referenced node - trying to get replication.");
        }
        return status;
      } catch (const ios_base::failure &error) {
        logger->error("[get] failed to open file {}, reason: {}", data, error.what());
        return Status::CANCELLED;
      }
    }
  }
  return Status::CANCELLED;
}

Status Service::get([[maybe_unused]] ServerContext *serverContext, const GetRequest *req, grpc::ServerWriter<GetResponse> *writer) {
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
    logger->debug("[get] file does not exist, trying to restore from metadata...");
    const auto status = get_from_reference_or_replication(uri);
    if(!status.ok()) {
      return status;
    }
  } else if (!file::is_regular_file(data)) {
    logger->error("[get] requested file is not a regular file - aborting.");
    return Status::CANCELLED;
  }

  logger->trace("[get] trying to get {}", data);
  try {
    file.open(data, fstream::binary);
  } catch (const ios_base::failure &error) {
    logger->error("[get] failed to open file {}, reason: {}", data, error.what());
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
      throw__exception("[get] broken stream.");
    }

  } while (read > 0);

  return Status::OK;
}

} //namespace fs
} //namespace chord
