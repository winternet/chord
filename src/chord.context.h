#pragma once

#include <memory>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

typedef boost::uuids::uuid uuid_t;

struct Context {

  uuid_t uuid { boost::uuids::random_generator()() };
  std::unique_ptr<uuid_t> predecessor { nullptr };
  std::unique_ptr<uuid_t> sucessor { nullptr };

};
