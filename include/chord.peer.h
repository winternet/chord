#pragma once

#include "chord.router.h"
#include "chord.client.h"
#include "chord.service.h"
#include "chord.i.scheduler.h"
#include <grpc++/server_builder.h>
#include "chord.queue.h"
#include "chord.uuid.h"


struct Context;
struct Router;
class AbstractScheduler;

using grpc::ServerBuilder;

class ChordPeer {
private:
  size_t next { 0 };
  chord::queue<std::string> commands;

  std::unique_ptr<AbstractScheduler> scheduler { nullptr };
  const std::shared_ptr<Context>& context;

  std::unique_ptr<Router> router        { nullptr };
  std::unique_ptr<ChordClient> client   { nullptr };
  std::unique_ptr<ChordService> service { nullptr };

  void start_server();
  void start_scheduler();

public:
  ChordPeer(const ChordPeer&) = delete;             // disable copying
  ChordPeer& operator=(const ChordPeer&) = delete;  // disable assignment

  ChordPeer(const std::shared_ptr<Context>& context);

  virtual ~ChordPeer();

  void start();

  /**
   * create new chord ring
   */
  void create();

  /**
   * join chord ring containing client-id.
   */
  void join();

  /**
   * successor
   */
  RouterEntry successor(const uuid_t& uuid);

  /**
   * stabilize the ring
   */
  void stabilize();

  /**
   * check predecessor
   */
  void check_predecessor();

  /**
   * fix finger table
   */
  void fix_fingers(size_t index);

  //-------------------------------------
  //---------BUSINESS LOGIC--------------
  //-------------------------------------
  /**
   * put
   */
  void put(const std::string& uri, std::istream& istream);
  /**
   * get
   */
  void get(const std::string& uri, std::ostream& ostream);

};
