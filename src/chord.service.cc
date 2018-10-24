#include <iostream>
#include <experimental/filesystem>

#include <grpc++/server_context.h>

#include "chord.log.h"
#include "chord.uri.h"
#include "chord.context.h"
#include "chord.router.h"

#include "chord.common.h"
#include "chord.client.h"
#include "chord.service.h"


using grpc::ServerContext;
using grpc::ClientContext;
using grpc::ServerBuilder;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::Status;

using chord::common::Header;
using chord::JoinResponse;
using chord::JoinRequest;
using chord::SuccessorResponse;
using chord::SuccessorRequest;
using chord::StabilizeResponse;
using chord::StabilizeRequest;
using chord::NotifyResponse;
using chord::NotifyRequest;
using chord::CheckResponse;
using chord::CheckRequest;
using chord::TakeResponse;
using chord::TakeRequest;
using chord::Chord;


namespace fs = std::experimental::filesystem::v1;
using namespace std;
using namespace chord::common;

namespace chord {

Service::Service(Context &context, Router *router, IClient* client)
    : context{context},
      router{router},
      client{client},
      logger{log::get_or_create(logger_name)} {}

Status Service::join(ServerContext *serverContext, const JoinRequest *req, JoinResponse *res) {
  (void)serverContext;
  const auto node = chord::common::make_node(req->header().src());
  //const auto id = req->header().src().uuid();
  //const auto endpoint = req->header().src().endpoint();

  logger->trace("join request from {}", node);
  /**
   * find successor
   */
  const auto& src = node.uuid; //uuid_t{id};

  const auto pred = router->predecessor();
  const auto succ = router->successor();
  //const auto entry = successor(src);

  /**
   * forward request to successor
   */
  if(pred && succ && !src.between(pred->uuid, context.uuid())) {
    return client->join(req, res);
  }

  const auto succ_or_self = succ.value_or(context.node());
  auto* res_succ = res->mutable_successor();
  res_succ->set_uuid(succ_or_self.uuid);
  res_succ->set_endpoint(succ_or_self.endpoint);

  const auto pred_or_self = pred.value_or(context.node());
  auto* res_pred = res->mutable_predecessor();
  res_pred->set_uuid(pred_or_self.uuid);
  res_pred->set_endpoint(pred_or_self.endpoint);

  res->mutable_header()->CopyFrom(make_header(context));

  /**
   * initialize the ring
   */
  if (!pred && (!succ || succ->uuid == context.uuid())) {
    logger->info("first node joining, setting the node as successor");
    router->set_successor(0, node);
    router->set_predecessor(0, node);
  }

  return Status::OK;
}

grpc::Status Service::take(ServerContext *serverContext,
                           const TakeRequest *req,
                           ServerWriter<TakeResponse> *writer) {
  (void)serverContext;
  if(!take_producer_callback) {
    logger->error("take callback is not set - aborting");
    return Status::CANCELLED;
  }
  TakeResponse res;
  const auto from = uuid_t{req->from()};
  const auto to = uuid_t{req->to()};
  const auto responses = take_producer_callback(from, to);

  for(const auto& res:responses) {
    if (!writer->Write(res)) {
      throw__exception("broken stream.");
    }
  }
  return Status::OK;
}

RouterEntry Service::successor(const uuid_t &uuid) {
  ServerContext serverContext;
  SuccessorRequest req;
  SuccessorResponse res;

  req.mutable_header()->CopyFrom(make_header(context));
  req.set_id(uuid);

  const auto status = successor(&serverContext, &req, &res);

  if (!status.ok()) throw__grpc_exception("failed to query successor", status);

  return res.successor();
}

Status Service::successor(ServerContext *serverContext, const SuccessorRequest *req, SuccessorResponse *res) {
  (void)serverContext;
  logger->trace("[successor] from {}@{} successor of? {}",
                req->header().src().uuid(), req->header().src().endpoint(),
                req->id());

  //--- destination of the request
  uuid_t id{req->id()};
  uuid_t self{context.uuid()};

  const auto successor = router->successor();

  if(!successor) {
    logger->error("failed to query successor from this->router");
    return Status::CANCELLED;
  }

  if(id == self || id.between(self, successor->uuid)) {
    logger->trace("[successor] the requested id {} lies between self {} and my successor {}, returning successor", id.string(), self.string(), successor->uuid);

    //--- router entry
    RouterEntry entry;
    entry.set_uuid(successor->uuid);
    entry.set_endpoint(successor->endpoint);

    res->mutable_successor()->CopyFrom(entry);
  } else {
    logger->trace("[successor] trying to spawn client to forward request.");
    const auto status = client->successor(req, res);
    if (!status.ok()) {
      logger->warn("[successor] failed to query successor from client!");
    } else {
      logger->trace("spawned client, got result");
      logger->trace("entry {}@{}", res->successor().uuid(), res->successor().endpoint());
    }
    return status;
  }

  return Status::OK;
}

Status Service::stabilize(ServerContext *serverContext, const StabilizeRequest *req, StabilizeResponse *res) {
  (void)serverContext;
  logger->trace("stabilize from {}@{}", req->header().src().uuid(),
                req->header().src().endpoint());

  const auto predecessor = router->predecessor();
  if (predecessor) {
    logger->debug("returning predecessor {}", predecessor);

    RouterEntry entry;
    entry.set_uuid(predecessor->uuid);
    entry.set_endpoint(predecessor->endpoint);

    res->mutable_predecessor()->CopyFrom(entry);
  }
  res->mutable_header()->CopyFrom(make_header(context));

  return Status::OK;
}

Status Service::leave(ServerContext *serverContext, const LeaveRequest *req, LeaveResponse *res) {
  (void)serverContext;
  (void)res;
  logger->trace("received leaving node {}@{}", req->header().src().uuid(),
                req->header().src().endpoint());

  //--- validate
  if (!req->has_header() && !req->header().has_src()) {
    logger->warn("failed to validate header");
    return Status::CANCELLED;
  }

  const uuid_t self{context.uuid()};
  // we trust the sender
  const uuid_t uuid{req->header().src().uuid()};
  const auto endpoint = req->header().src().endpoint();

  const uuid_t from_uuid{req->predecessor().uuid()};
  const node to{req->header().src().uuid(), req->header().src().endpoint()};

  //const uuid to_uuid = {req->header().src().uuid()};
  //const node from{req->predecessor().uuid(), req->predecessor().endpoint()};

  /**
   * TODO
   * 1) send TakeRequest to source
   * 2) consumer needs to issue getRequests for each metadata
   */
  //const auto status = client->take(req, res);
  if(!on_leave_callback) {
    logger->warn("on leave callback not set - aborting.");
    return Status::CANCELLED;
  }

  client->take(from_uuid, to.uuid, to, on_leave_callback);
  return Status::OK;
}

Status Service::notify(ServerContext *serverContext, const NotifyRequest *req, NotifyResponse *res) {
  (void)serverContext;
  (void)res;
  logger->trace("notify from {}@{}", req->header().src().uuid(),
                req->header().src().endpoint());

  const auto predecessor = router->predecessor();

  //--- validate
  if (!req->has_header() && !req->header().has_src()) {
    logger->warn("failed to validate header");
    return Status::CANCELLED;
  }

  const uuid_t self{context.uuid()};
  const auto node = chord::common::make_node(req->header().src());
  //const uuid_t uuid{req->header().src().uuid()};
  //const auto endpoint = req->header().src().endpoint();

  if (!predecessor
      || node.uuid.between(predecessor->uuid, self)) {
    router->set_predecessor(0, node);
  } else {
    if (!predecessor)
      logger->info("predecessor is null");
    else {
      logger->info("n' = {}\npredecessor = {}\nself={}", node.uuid, predecessor, self);
    }
  }

  return Status::OK;
}

Status Service::check(ServerContext *serverContext, const CheckRequest *req, CheckResponse *res) {
  (void)serverContext;
  logger->trace("check from {}@{}", req->header().src().uuid(),
                req->header().src().endpoint());
  res->mutable_header()->CopyFrom(make_header(context));
  res->set_id(req->id());
  return Status::OK;
}

void Service::fix_fingers(size_t index) {
  auto fix = context.uuid();
  if (!router->successor(0)) {
    fix += uuid_t{pow(2., (double) index - 1)};
    logger->trace("fix_fingers router successor is not null, increasing uuid");
  }

  logger->trace("fixing finger for {}.", fix);

  try {
    const auto succ = chord::common::make_node(successor(fix));
    logger->trace("fixing finger for {}. received successor {}", fix, succ);
    if( succ.uuid == context.uuid() ) {
      router->reset(fix);
      return;
    } 

    router->set_successor(index, succ);
  } catch (const chord::exception &e) {
    logger->warn("failed to fix fingers for {}", fix);
  }
}

} //namespace chord
