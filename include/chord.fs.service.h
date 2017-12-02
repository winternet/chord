#pragma once
#include <functional>

#include <grpc/grpc.h>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include "chord.context.h"
#include "chord_fs.grpc.pb.h"
#include "chord.i.service.h"
#include "chord.exception.h"

#include "chord.client.h"

namespace chord {
  namespace fs {

    //typedef std::function<chord::client()> ClientFactory;
    class Service final : public chord::fs::Filesystem::Service {

      public:
        Service(Context& context);

        //Service(const std::shared_ptr<Context>& context, ClientFactory make_client);

        virtual grpc::Status put(grpc::ServerContext* context, grpc::ServerReader<chord::fs::PutRequest>* reader, chord::fs::PutResponse* response) override;

        virtual grpc::Status get(grpc::ServerContext* context, const chord::fs::GetRequest* req, grpc::ServerWriter<chord::fs::GetResponse>* writer) override;

      private:
        Context& context;
    };

  } //namespace fs
} //namespace chord
