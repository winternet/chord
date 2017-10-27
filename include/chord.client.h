#pragma once

#include <memory>
#include <functional>

#include "chord.log.h"
#include "chord.uuid.h"
#include "chord.router.h"
#include "chord.context.h"
#include "chord.grpc.pb.h"


typedef std::function<std::unique_ptr<chord::Chord::StubInterface>(const endpoint_t& endpoint)> StubFactory;

class ChordClient {
private:
  Context& context;
  Router& router;

  StubFactory make_stub;

  /**
   * return new header
   */
  chord::Header make_header();
  chord::SuccessorRequest make_request();

public:
  ChordClient(Context& context, Router& router);
  ChordClient(Context& context, Router& router, StubFactory factory);

  void join(const endpoint_t& addr);

  void stabilize();
  void notify();
  void check();
  void fix_fingers();

  chord::RouterEntry successor(const uuid_t& id);
  grpc::Status successor(grpc::ClientContext* context, const chord::SuccessorRequest* req, chord::SuccessorResponse* res);
  grpc::Status successor(const chord::SuccessorRequest* req, chord::SuccessorResponse* res);
  grpc::Status put(const std::string& uri, std::istream& istream);
};
