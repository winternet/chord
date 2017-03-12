#pragma once

#include <stdexcept>

#include "chord.router.h"
#include "chord.client.h"
#include "chord.service.h"

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>

using grpc::ServerBuilder;

class ChordPeer {
private:
  std::shared_ptr<Router> router { nullptr };
  std::shared_ptr<Context> context { nullptr };
  std::unique_ptr<ChordClient> client { nullptr };
  std::unique_ptr<ChordServiceImpl> service { nullptr };

  void start_server() {
    endpoint_t bind_addr = context->bind_addr;
    ServerBuilder builder;
    builder.AddListeningPort(bind_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());
  
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    LOG(debug) << "server listening on " << bind_addr;
    server->Wait();
  }

public:
  ChordPeer(std::shared_ptr<Context> context) 
    : context { context }
    , router  { context->router }
    , client  { new ChordClient{context} }
    , service { new ChordServiceImpl{context} }
  {
    LOG(trace) << "peer with client-id " << context->uuid;
    if( context->bootstrap ) {
      create();
    } else {
      join();
    }
    start_server();
  }

  virtual ~ChordPeer() {}

  /**
   * join chord ring containing client-id.
   */
  void join() {
    //context->predecessor = nullptr;
    client->join(context->join_addr);
  }

  /**
   * find successor
   */

  /**
   * closest preceeding node
   */

  /**
   * create new chord ring
   */
  void create() {
    LOG(trace) << "bootstrapping new chord ring.";
    //context->predecessor = nullptr;
    context->router->set_successor(
        context->uuid,
        context->bind_addr);
  }
};

