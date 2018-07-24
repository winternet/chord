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

#include "chord.common.h"
#include "chord.client.h"
#include "chord.fs.service.h"
#include "chord.concurrent.queue.h"

using grpc::ServerContext;
using grpc::ClientContext;
using grpc::ServerBuilder;
using grpc::ServerReader;
using grpc::Status;
using grpc::StatusCode;

using chord::common::Header;
using chord::fs::PutResponse;
using chord::fs::PutRequest;
using chord::fs::GetResponse;
using chord::fs::GetRequest;


using namespace std;
using namespace chord;
using namespace chord::common;

namespace chord {
namespace fs {

Service::Service(Context &context, chord::ChordFacade *chord)
    : context{context},
      chord{chord},
      metadata{make_unique<MetadataManager>(context)},
      make_client {[this]{
        return chord::fs::Client(this->context, this->chord);
      }},
      logger{log::get_or_create(logger_name)} {
        //consumer = std::thread([&]{
        //    logger->debug("starting consumer thread for take");
        //    //chord->
        //});
      }

Status Service::meta(ServerContext *serverContext, const MetaRequest *req, MetaResponse *res) {
  (void)serverContext;

  const auto uri = uri::from(req->uri());

  try {
    switch (req->action()) {
      case ADD:
        metadata->add(uri, MetadataBuilder::from(req));
        break;
      case DEL:
        metadata->del(uri, MetadataBuilder::from(req));
        break;
      case MOD:
        return Status::CANCELLED;
        break;
      case DIR:
        set<Metadata> meta = metadata->get(uri);
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

  // open
  if (reader->Read(&req)) {
    const auto uri = uri::from(req.uri());

    data /= uri.path().parent_path();

    if (!file::exists(data)) {
      logger->trace("creating directories for {}", data);
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
        file.write((const char *)req.data().data(), req.size());
      } while (reader->Read(&req));

    } catch (const ios_base::failure &error) {
      logger->error("failed to open file {}, reason: {}", data, error.what());
      return Status::CANCELLED;
    }
  }

  // metadata
  try {
    const auto uri = uri::from(req.uri());

    // remote / local(last)
    for (const path &path : uri.path().all_paths()) {
      const auto sub_uri = uri::builder{uri.scheme(), path}.build();
      auto meta = MetadataBuilder::for_path(context, path);
      make_client().meta(sub_uri, Client::Action::ADD, meta);
    }
  } catch(const chord::exception &e) {
    logger->error("failed to add metadata: {}", e.what());
    file::remove(data);
    throw;
  }
  return Status::OK;
}

Status Service::del(grpc::ServerContext *serverContext, const chord::fs::DelRequest *req,
           chord::fs::DelResponse *res) {
  (void)res;
  (void)serverContext;

  path data = context.data_directory;
  //TODO support directories
  if (!file::is_directory(data)) {
    return Status::CANCELLED;
  }

  const auto uri = chord::uri::from(req->uri());

  data /= uri.path();

  if(file::exists(data)) {
    file::remove_all(data);
  } else {
    return Status::CANCELLED;
    //TODO look on some other node (e.g. the successor that this node initially joined)
    //     maybe some other node is responsible for that file recently
  }

  // local
  metadata->del(uri);
  // remote
  const auto status = make_client().meta(uri, Client::Action::DEL);
  if (!status.ok()) {
    //TODO handle error
  }

  return Status::OK;
}

Status Service::get(ServerContext *serverContext, const GetRequest *req, grpc::ServerWriter<GetResponse> *writer) {
  (void)serverContext;
  ifstream file;

  path data = context.data_directory;
  if (!file::is_directory(data)) {
    return Status::CANCELLED;
  }

  const auto uri = chord::uri::from(req->uri());

  data /= uri.path();
  if (!file::is_regular_file(data)) {
    return Status::CANCELLED;
  }

  logger->trace("trying to get {}", data);
  file.exceptions(ifstream::failbit | ifstream::badbit);
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
    read = file.readsome(buffer, len);
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
