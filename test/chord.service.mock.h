#pragma once

#include "chord.i.service.h"

namespace chord {
class MockService : public IService {
  public:

  MOCK_METHOD3(join, grpc::Status(grpc::ServerContext *context,
                            const chord::JoinRequest *req,
                            chord::JoinResponse *res));

  MOCK_METHOD1(successor, chord::common::RouterEntry(const uuid_t &uuid));


  MOCK_METHOD3(successor, grpc::Status(grpc::ServerContext *context,
                                 const chord::SuccessorRequest *req,
                                 chord::SuccessorResponse *res));

  MOCK_METHOD3(stabilize, grpc::Status(grpc::ServerContext *context,
                                 const chord::StabilizeRequest *req,
                                 chord::StabilizeResponse *res));

  MOCK_METHOD3(notify, grpc::Status(grpc::ServerContext *context,
                              const chord::NotifyRequest *req,
                              chord::NotifyResponse *res));

  MOCK_METHOD3(check, grpc::Status(grpc::ServerContext *context,
                             const chord::CheckRequest *req,
                             chord::CheckResponse *res));

  MOCK_METHOD3(leave, grpc::Status(grpc::ServerContext *context,
                             const chord::LeaveRequest *req,
                             chord::LeaveResponse *res));

  MOCK_METHOD1(fix_fingers, void(size_t index));

  MOCK_METHOD0(on_leave, signal<void(const node, const node)>&(void));

  MOCK_METHOD0(on_predecessor_update, signal<void(const node, const node)>&(void));

  MOCK_METHOD0(grpc_service, ::grpc::Service*(void));

};
}
