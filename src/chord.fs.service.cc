#include <iostream>
#include <fstream>
#include <experimental/filesystem>
#include <grpc/grpc.h>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include "chord.log.h"
#include "chord.uri.h"
#include "chord.context.h"
#include "chord_fs.grpc.pb.h"

#include "chord.common.h"
#include "chord.client.h"
#include "chord.fs.service.h"

#define log(level) LOG(level) << "[service] "

#define SERVICE_LOG(level,method) LOG(level) << "[service][" << #method << "] "

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


namespace filesystem = std::experimental::filesystem::v1;
using namespace std;
using namespace chord::common;

namespace chord {
  namespace fs {

    Service::Service(Context& context)
      : context{ context }
    {
    }

    Status Service::put(ServerContext* serverContext, ServerReader<PutRequest>* reader, PutResponse* response) {
      PutRequest req;
      ofstream file;

      //TODO make configurable
      filesystem::path data {"./data"};
      if( !filesystem::is_directory(data) ) {
        filesystem::create_directory(data);
      }

      // open
      if(reader->Read(&req)) {
        auto id = req.id();
        auto uri = chord::uri::from(req.uri());

        data /= uri.directory();
        if( !filesystem::is_directory(data) ) {
          SERVICE_LOG(trace, put) << "creating directories for " << data;
          filesystem::create_directories(data);
        }

        data /= uri.filename();
        SERVICE_LOG(trace, put) << "trying to put " << data;
        file.exceptions(ifstream::failbit | ifstream::badbit);
        //TODO make data path configurable
        //TODO auto-create parent paths if not exist
        try {
          file.open(data, std::fstream::binary);
        } catch(ios_base::failure error) {
          SERVICE_LOG(error, put) << "failed to open file " << data << ", " << error.what();
          return Status::CANCELLED;
        }
      }

      //write
      do {
        file.write((const char*)req.data().data(), req.size());
      } while(reader->Read(&req));

      // close
      file.close();
      return Status::OK;
    }

    Status Service::get(grpc::ServerContext* context, const GetRequest* req, grpc::ServerWriter<GetResponse>* writer) {
      ifstream file;

      //TODO make configurable
      filesystem::path data {"./data"};
      if( !filesystem::is_directory(data) ) {
        return Status::CANCELLED;
      }

      auto id = req->id();
      auto uri = chord::uri::from(req->uri());

      data /= uri.path();
      if( !filesystem::is_regular_file(data) ) {
        return Status::CANCELLED;
      }

      SERVICE_LOG(trace, put) << "trying to get " << data;
      file.exceptions(ifstream::failbit | ifstream::badbit);
      try {
        file.open(data, std::fstream::binary);
      } catch(ios_base::failure error) {
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
        if(read <= 0) break;

        GetResponse res;
        //TODO validate hashes
        res.set_id(req->id());
        res.set_data(buffer, read);
        res.set_offset(offset);
        res.set_size(read);
        //TODO write implicit string conversion
        //res.set_uri(uri);
        offset += read;

        if(!writer->Write(res)) {
          throw chord::exception("broken stream.");
        }

      } while(read > 0);

      return Status::OK;
    }

  } //namespace fs
} //namespace chord
