#pragma once

#include "chord.pb.h"
#include "chord.context.h"


namespace chord {

struct node;

namespace common {

chord::common::Header make_header(const chord::node&);

chord::common::Header make_header(const Context*);

chord::common::Header make_header(const Context&);

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

chord::node make_node(const RouterEntry&);

RouterEntry make_entry(const node&);

RouterEntry make_entry(const uuid &, const endpoint&);

}
}
