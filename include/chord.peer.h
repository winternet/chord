#pragma once

#include "chord.i.scheduler.h"
#include <grpc++/server_builder.h>
//
#include "chord.queue.h"
#include "chord.uuid.h"


struct Context;
struct Router;
class AbstractScheduler;
class ChordClient;
class ChordServiceImpl;

using grpc::ServerBuilder;

class ChordPeer {
private:
  size_t next { 0 };
  chord::queue<std::string> commands;

  std::unique_ptr<AbstractScheduler> scheduler { nullptr };
  std::shared_ptr<Context> context      { nullptr };

  std::unique_ptr<Router> router        { nullptr };
  std::unique_ptr<ChordClient> client   { nullptr };
  std::unique_ptr<ChordServiceImpl> service { nullptr };

  void start_server();

public:
  ChordPeer(const ChordPeer&) = delete;             // disable copying
  ChordPeer& operator=(const ChordPeer&) = delete;  // disable assignment

  ChordPeer(std::shared_ptr<Context> context);

  virtual ~ChordPeer();

  void start_scheduler();

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
  void successor(const uuid_t& uuid);

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
  void put(std::istream& istream);

};
