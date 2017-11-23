#include "chord.context.h"
#include "chord.grpc.pb.h"

using chord::Header;

namespace chord {
namespace common {

  chord::Header make_header(const Context& context) {
    Header header;
    RouterEntry src;
    src.set_uuid(context.uuid());
    src.set_endpoint(context.bind_addr);
    header.mutable_src()->CopyFrom(src);
    return header;
  }

}
}
