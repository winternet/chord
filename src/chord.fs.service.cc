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

#define log(level) LOG(level) << "[service] "

#define SERVICE_LOG(level, method) LOG(level) << "[service][" << #method << "] "

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
    : context{context} {
}

Status Service::notify(ServerContext *serverContext, const NotifyRequest *request, NotifyResponse *response) {
  return Status::OK;
}

Status Service::put(ServerContext *serverContext, ServerReader<PutRequest> *reader, PutResponse *response) {
  PutRequest req;
  ofstream file;

  //TODO make configurable
  path data{"./data"};
  if (!file::is_directory(data)) {
    file::create_directory(data);
  }

  // open
  if (reader->Read(&req)) {
    const auto uri = uri::from(req.uri());

    data /= uri.path().parent_path();
    if (!file::is_directory(data)) {
      SERVICE_LOG(trace, put) << "creating directories for " << data;
      file::create_directories(data);
    }

    data /= uri.path().filename();
    SERVICE_LOG(trace, put) << "trying to put " << data;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    //TODO make data path configurable
    //TODO auto-create parent paths if not exist
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
  return Status::OK;
}

Status Service::get(ServerContext *context, const GetRequest *req, grpc::ServerWriter<GetResponse> *writer) {
  ifstream file;

  //TODO make configurable
  path data{"./data"};
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
