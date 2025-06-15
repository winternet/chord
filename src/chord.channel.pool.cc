#include "chord.channel.pool.h"

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "chord.context.h"
#include "chord.log.factory.h"
#include "chord.log.h"
#include "chord.node.h"
#include "chord.types.h"
#include "chord.uuid.h"

namespace chord {

ChannelPool::ChannelPool(chord::Context &context)
  : context{context}
  , logger{context.logging.factory().get_or_create(logger_name)}
  {}

void ChannelPool::put(const node& node, std::shared_ptr<grpc::Channel> channel) {
  channels.put(node.uuid, channel);
}

std::shared_ptr<grpc::Channel> ChannelPool::get(const node& node) {
  return channels.compute_if_absent(node.uuid, [&]([[maybe_unused]] const uuid& key) { return ChannelPool::create_channel(node.endpoint); });
}

std::shared_ptr<grpc::Channel> ChannelPool::create_channel(const endpoint& endpoint) {
 return grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials());
}

} // namespace chord
