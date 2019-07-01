#pragma once

#include "chord.pb.h"
#include "chord.context.h"

namespace chord {

struct node;

namespace common {

chord::common::Header make_header(const Context *context);

chord::common::Header make_header(const Context &context);

chord::node make_node(const RouterEntry& entry);

}
}
