#pragma once

#include <grpc++/server_context.h>
#include <grpc/grpc.h>
#include <functional>
#include <vector>

#include "chord.exception.h"
#include "chord.grpc.pb.h"
#include "chord.i.client.h"
#include "chord.i.service.h"
#include "chord.pb.h"
#include "chord.uuid.h"
#include "chord.signal.h"

namespace chord { class Context; }
namespace chord { struct Router; }
namespace chord { class IClient; }

namespace chord {
class IService {
 public:
  virtual ~IService() = default;

  virtual grpc::Status join(grpc::ServerContext *context,
                            const chord::JoinRequest *req,
                            chord::JoinResponse *res) = 0;

  virtual chord::common::RouterEntry successor(const uuid_t &uuid) = 0;


  virtual grpc::Status successor(grpc::ServerContext *context,
                                 const chord::SuccessorRequest *req,
                                 chord::SuccessorResponse *res) = 0;

  virtual grpc::Status stabilize(grpc::ServerContext *context,
                                 const chord::StabilizeRequest *req,
                                 chord::StabilizeResponse *res) = 0;

  virtual grpc::Status notify(grpc::ServerContext *context,
                              const chord::NotifyRequest *req,
                              chord::NotifyResponse *res) = 0;

  virtual grpc::Status check(grpc::ServerContext *context,
                             const chord::CheckRequest *req,
                             chord::CheckResponse *res) = 0;

  virtual grpc::Status leave(grpc::ServerContext *context,
                             const chord::LeaveRequest *req,
                             chord::LeaveResponse *res) = 0;

  virtual void fix_fingers(size_t index) = 0;

  virtual signal<void(const node, const node)>& on_joined() = 0;

  virtual ::grpc::Service* grpc_service() = 0;

};
} // namespace chord
