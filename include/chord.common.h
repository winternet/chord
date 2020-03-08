#pragma once

#include "chord.pb.h"
#include "chord.context.h"

namespace chord {

struct node;

namespace common {

chord::common::Header make_header(const chord::node &node);

chord::common::Header make_header(const Context *context);

chord::common::Header make_header(const Context &context);

chord::node make_node(const RouterEntry& entry);

RouterEntry make_entry(const chord::node& node);

template<typename T>
chord::node source_of(const T* req) {
  return make_node(req->header().src());
}
template<typename T>
void set_source(T* res, const Context& context) {
  res->mutable_header()->CopyFrom(make_header(context));
}

template<typename T>
void set_source(T& res, const Context& context) {
  res.mutable_header()->CopyFrom(make_header(context));
}


}
}
