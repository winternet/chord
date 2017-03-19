#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "chord.service.h"

std::shared_ptr<Context> make_context(const uuid_t& self) {
  std::shared_ptr<Context> context = std::shared_ptr<Context>(new Context);
  context->set_uuid(self);
  
  return context;
}

RouterEntry make_entry(const uuid_t& id, const endpoint_t& addr) {
  RouterEntry entry;
  entry.set_uuid(to_string(id));
  entry.set_endpoint(addr);
  return entry;
}

Header make_header(const uuid_t& id, const endpoint_t& addr) {
  Header header;
  header.mutable_src()->CopyFrom(make_entry(id, addr));
  return header;
}

TEST(ServiceTest, join) {
  std::shared_ptr<Context> context = std::shared_ptr<Context>(new Context);
  ChordServiceImpl service(context);

  //TODO assertions and request

  //ServerContext serverContext;
  //JoinRequest req;
  //JoinResponse res;
  //service.join(&serverContext, &req, &res);
}

/**
 * ring with 1 node
 *   - 0 @ 0.0.0.0:50050
 * node 0 tries to find id 10 -> self
 */
TEST(ServiceTest, successor_single_node) {
  std::shared_ptr<Context> context = make_context(0);
  std::shared_ptr<Router> router = context->router;

  ChordServiceImpl service(context);

  ServerContext serverContext;
  SuccessorRequest req;
  SuccessorResponse res;

  req.mutable_header()->CopyFrom(make_header(0, "0.0.0.0:50050"));
  req.set_id(to_string(10));

  service.successor(&serverContext, &req, &res);

  ASSERT_EQ(uuid_t(res.successor().uuid()), 0);
  ASSERT_EQ(res.successor().endpoint(), "0.0.0.0:50050");
}

/**
 * ring with 2 nodes
 *   - 0 @ 0.0.0.0:50050
 *   - 5 @ 0.0.0.0:50055
 * node 0 tries to find successor of id 2 -> 5
 */
TEST(ServiceTest, successor_two_nodes) {
  std::shared_ptr<Context> context = make_context(0);

  std::shared_ptr<Router> router = context->router;
  router->set_successor(0, 5, "0.0.0.0:50055");
  router->set_predecessor(0, 5, "0.0.0.0:50055");

  ChordServiceImpl service(context);

  ServerContext serverContext;
  SuccessorRequest req;
  SuccessorResponse res;

  req.mutable_header()->CopyFrom(make_header(0, "0.0.0.0:50050"));
  req.set_id(to_string(2));

  service.successor(&serverContext, &req, &res);

  ASSERT_EQ(uuid_t(res.successor().uuid()), 5);
  ASSERT_EQ(res.successor().endpoint(), "0.0.0.0:50055");
}

/**
 * ring with 2 nodes
 *   - 0 @ 0.0.0.0:50050
 *   - 5 @ 0.0.0.0:50055
 * node 5 tries to find successor of id 2 -> 5
 */
TEST(ServiceTest, successor_two_nodes_modulo) {
  std::shared_ptr<Context> context = make_context(5);

  std::shared_ptr<Router> router = context->router;
  router->set_successor(0, 0, "0.0.0.0:50050");
  router->set_predecessor(0, 0, "0.0.0.0:50050");

  ChordServiceImpl service(context);

  ServerContext serverContext;
  SuccessorRequest req;
  SuccessorResponse res;

  req.mutable_header()->CopyFrom(make_header(5, "0.0.0.0:50055"));
  req.set_id(to_string(2));

  service.successor(&serverContext, &req, &res);

  ASSERT_EQ(uuid_t(res.successor().uuid()), 5);
  ASSERT_EQ(res.successor().endpoint(), "0.0.0.0:50055");
}
