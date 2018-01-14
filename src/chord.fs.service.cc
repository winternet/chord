#include <iostream>
#include <fstream>
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

#define SERVICE_LOG(level, method) LOG(level) << "[fs.service][" << #method << "] "

using grpc::ServerContext;
using grpc::ClientContext;
using grpc::ServerBuilder;
using grpc::ServerReader;
using grpc::Status;

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

Service::Service(Context *context)
    : context{context}, metadata{make_unique<MetadataManager>(context)} {
}

Status Service::meta(ServerContext *serverContext, const MetaRequest *req, MetaResponse *res) {
  (void)serverContext;
  if(!req->has_metadata()) {
    throw chord::exception("Failed to process meta request: metadata is missing.");
  }
  SERVICE_LOG(trace, notify) 
    << "filename=" << req->metadata().filename()
    << ", uri=" << req->uri();

  auto uri = uri::from(req->uri());

	switch(req->action()) {
	case ADD: 
    metadata->add(uri, MetadataBuilder::from(req));
		break;
	case DEL: 
    metadata->del(uri, MetadataBuilder::from(req));
		break;
  case MOD: 
    return Status::CANCELLED;
		break;
  case LST:
    return Status::CANCELLED;
    break;
	}

  return Status::OK;
}

Status Service::put(ServerContext *serverContext, ServerReader<PutRequest> *reader, PutResponse *response) {
  (void)serverContext;
  (void)response;
  PutRequest req;
  ofstream file;

  path data = context->data_directory;
  if (!file::is_directory(data)) {
    file::create_directories(data);
  }

  const auto uri = uri::from(req.uri());

  // open
  if (reader->Read(&req)) {

    data /= uri.path().parent_path();
    if (!file::exists(data)) {
      SERVICE_LOG(trace, put) << "creating directories for " << data;
      file::create_directories(data);
    }

    data /= uri.path().filename();
    SERVICE_LOG(trace, put) << "trying to put " << data;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    try {
      file.open(data, fstream::binary);
    } catch (const ios_base::failure& error) {
      SERVICE_LOG(error, put) << "failed to open file " << data << ", " << error.what();
      return Status::CANCELLED;
    }
  }

  //write
  do {
    file.write((const char *) req.data().data(), req.size());
  } while (reader->Read(&req));

  // close
  file.close();

  // metadata
  try {
    metadata->add(uri, {uri.path().filename()});
  } catch(const chord::exception &e) {
    SERVICE_LOG(error, put) << "failed to add metadata: " << e.what();
    file::remove(data);
    throw e;
  }
  return Status::OK;
}

Status Service::get(ServerContext *serverContext, const GetRequest *req, grpc::ServerWriter<GetResponse> *writer) {
  (void)serverContext;
  ifstream file;

  path data = context->data_directory;
  if (!file::is_directory(data)) {
    return Status::CANCELLED;
  }

  auto uri = chord::uri::from(req->uri());

  data /= uri.path();
  if (!file::is_regular_file(data)) {
    return Status::CANCELLED;
  }

  SERVICE_LOG(trace, put) << "trying to get " << data;
  file.exceptions(ifstream::failbit | ifstream::badbit);
  try {
    file.open(data, fstream::binary);
  } catch (const ios_base::failure &error) {
    SERVICE_LOG(error, put) << "failed to open file " << data << ", " << error.what();
    return Status::CANCELLED;
  }

  //TODO make configurable (see chord.client)
  constexpr size_t len = 512*1024; // 512k
  char buffer[len];
  size_t offset = 0,
      read = 0;
  //read
  do {
    read = file.readsome(buffer, len);
    if (read <= 0) break;

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
      throw chord::exception("broken stream.");
    }

  } while (read > 0);

  return Status::OK;
}

} //namespace fs
} //namespace chord
