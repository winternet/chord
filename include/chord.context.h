#pragma once

#include <memory>

#include "chord.uuid.h"

uuid_t generate_random();

struct Context {

  uuid_t uuid {generate_random()};

  std::unique_ptr<uuid_t> predecessor { nullptr };
  std::unique_ptr<uuid_t> sucessor { nullptr };

};
