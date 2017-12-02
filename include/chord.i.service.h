#pragma once

#include "chord.uuid.h"
#include "chord.grpc.pb.h"

//using chord::RouterEntry;
//
//using grpc::Status;
//using grpc::ServerContext;
//
//using chord::SuccessorResponse;
//using chord::SuccessorRequest;

class AbstractService {
  public:
    virtual chord::common::RouterEntry successor(const uuid_t& uuid) noexcept(false) =0;
    virtual grpc::Status successor(grpc::ServerContext* serverContext, const chord::SuccessorRequest* req, chord::SuccessorResponse* res) =0;

    virtual ~AbstractService() {}
};
