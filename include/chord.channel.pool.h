#pragma once
#include <cstddef>
#include <memory>
#include <mutex>
#include <grpcpp/channel.h>
#include <grpcpp/completion_queue.h>

#include "chord.node.h"
#include "chord.router.h"
#include "chord.types.h"
#include "chord.lru.cache.h"
#include "chord.uuid.h"

namespace chord { struct Context; }

namespace spdlog {
  class logger;
}

namespace chord {

class ChannelPool {
public:
  using mutex_t = std::recursive_mutex;

protected:
  chord::LRUCache<uuid_t, std::shared_ptr<grpc::Channel>> channels{Router::BITS};

private:
  static constexpr auto logger_name = "chord.channel.pool";

  chord::Context &context;
  std::shared_ptr<spdlog::logger> logger;

  mutable mutex_t mtx;

public:
  explicit ChannelPool(chord::Context &context);
  explicit ChannelPool(const chord::ChannelPool&) = delete;
  virtual ~ChannelPool() = default;
  ChannelPool& operator=(const ChannelPool&) = delete;

  void put(const node& node, std::shared_ptr<grpc::Channel> channel);
  //std::shared_ptr<grpc::Channel> get(const endpoint&);
  std::shared_ptr<grpc::Channel> get(const node&);

  static std::shared_ptr<grpc::Channel> create_channel(const endpoint&);
};

}  // namespace chord
