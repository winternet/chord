#pragma once
#include <cstddef>
#include <array>
#include <iosfwd>
#include <map>
#include <set>
#include <memory>
#include <future>
#include <mutex>
#include <grpcpp/channel.h>
#include <grpcpp/completion_queue.h>

#include "chord.signal.h"
#include "chord.router.h"
#include "chord.types.h"
#include "chord.optional.h"
#include "chord.node.h"
#include "chord.uuid.h"
#include "chord.lru.cache.h"

namespace chord { struct Context; }

namespace spdlog {
  class logger;
}

namespace chord {

class ChannelPool {
public:
  using mutex_t = std::recursive_mutex;

protected:
  chord::LRUCache<endpoint, std::shared_ptr<grpc::Channel>> channels{Router::BITS};

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

  void put(const endpoint& endpoint, std::shared_ptr<grpc::Channel> channel);
  std::shared_ptr<grpc::Channel> get(const endpoint&);
};

}  // namespace chord
