#pragma once

#include "chord.uuid.h"
#include "chord.grpc.pb.h"

using chord::RouterEntry;

using grpc::Status;
using grpc::ServerContext;

using chord::SuccessorResponse;
using chord::SuccessorRequest;

class AbstractService {
  public:
    virtual RouterEntry successor(const uuid_t& uuid) noexcept(false) =0;
    virtual Status      successor(ServerContext* serverContext, const SuccessorRequest* req, SuccessorResponse* res) =0;

    virtual ~AbstractService() {}
};
