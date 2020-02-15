#pragma once

#include "chord.i.service.h"

namespace chord {
class MockService : public IService {
  public:

  MOCK_METHOD3(ping, grpc::Status(grpc::ServerContext *context,
                            const chord::PingRequest *req,
                            chord::PingResponse *res));

  MOCK_METHOD1(lookup, chord::common::RouterEntry(const uuid_t &uuid));


  MOCK_METHOD3(lookup, grpc::Status(grpc::ServerContext *context,
                                 const chord::LookupRequest *req,
                                 chord::LookupResponse *res));

  MOCK_METHOD3(notify, grpc::Status(grpc::ServerContext *context,
                              const chord::NotifyRequest *req,
                              chord::NotifyResponse *res));

  MOCK_METHOD3(leave, grpc::Status(grpc::ServerContext *context,
                             const chord::LeaveRequest *req,
                             chord::LeaveResponse *res));

  MOCK_METHOD1(fix_fingers, void(size_t index));

  MOCK_METHOD0(on_leave, signal<void(const node, const node)>&(void));

  MOCK_METHOD0(on_joined, signal<void(const node, const node)>&(void));

  MOCK_METHOD0(grpc_service, ::grpc::Service*(void));

};
}
