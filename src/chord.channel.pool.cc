#include "chord.channel.pool.h"

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/completion_queue.h>

#include <algorithm>
#include <set>
#include <array>
#include <iterator>
#include <ostream>
#include <memory>
#include <string>
#include <thread>

#include "chord.context.h"
#include "chord.log.factory.h"
#include "chord.log.h"
#include "chord.node.h"
#include "chord.types.h"
#include "chord.optional.h"
#include "chord.uuid.h"

namespace chord {

ChannelPool::ChannelPool(chord::Context &context)
  : context{context}
  , logger{context.logging.factory().get_or_create(logger_name)}
  {}

void ChannelPool::put(const endpoint& endpoint, std::shared_ptr<grpc::Channel> channel) {
  channels.put(endpoint, channel);
}

std::shared_ptr<grpc::Channel> ChannelPool::get(const endpoint& endpoint) {
  return channels.compute_if_absent(endpoint, [](const chord::endpoint& e) { return grpc::CreateChannel(e, grpc::InsecureChannelCredentials());});
}

} // namespace chord
