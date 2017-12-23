#pragma once

#include "chord.uuid.h"
#include "chord.grpc.pb.h"

class AbstractService {
 public:
  virtual chord::common::RouterEntry successor(const uuid_t &uuid) noexcept(false) =0;

  virtual grpc::Status
  successor(grpc::ServerContext *serverContext, const chord::SuccessorRequest *req, chord::SuccessorResponse *res) =0;

  virtual ~AbstractService() {}
};
