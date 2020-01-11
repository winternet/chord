#pragma once

#include "chord.pb.h"
#include "chord.context.h"

namespace chord {

struct node;

namespace common {

chord::common::Header make_header(const chord::node&);

chord::common::Header make_header(const Context*);

chord::common::Header make_header(const Context&);

chord::node make_node(const RouterEntry&);

RouterEntry make_entry(const node&);

RouterEntry make_entry(const uuid &, const endpoint&);

}
}
