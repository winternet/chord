#pragma once

#include "chord.i.client.h"

namespace chord {
class MockClient : public IClient {
  public:

  MOCK_METHOD0(leave, void(void));

  MOCK_METHOD1(join, grpc::Status(const endpoint&));

  MOCK_METHOD2(join, grpc::Status(const JoinRequest *req, JoinResponse *res));
  MOCK_METHOD3(join, grpc::Status(grpc::ClientContext *clientContext, const JoinRequest *req, JoinResponse *res));

  MOCK_METHOD1(ping, grpc::Status(const node&));

  MOCK_METHOD0(stabilize, void(void));

  MOCK_METHOD0(notify, grpc::Status(void));
  MOCK_METHOD3(notify, grpc::Status(const node&, const node&, const node&));

  MOCK_METHOD0(check, void(void));

  MOCK_METHOD1(successor, chord::common::RouterEntry(const uuid_t &id));

  MOCK_METHOD3(successor, grpc::Status(grpc::ClientContext *context, const chord::SuccessorRequest *req, chord::SuccessorResponse *res));

  MOCK_METHOD2(successor, grpc::Status(const chord::SuccessorRequest *req, chord::SuccessorResponse *res));
  
  MOCK_METHOD0(on_successor_fail, signal<const node>&(void));

  MOCK_METHOD0(on_predecessor_fail, signal<const node>&(void));

};
}
