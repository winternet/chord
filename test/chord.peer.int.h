#pragma once

#include <iosfwd>
#include <grpcpp/impl/codegen/status.h>

#include "chord.peer.h"
#include "chord.controller.client.h"

namespace chord {

/**
 * sub-class of Peer that exposes the internal facades for integration tests.
 */
class IntPeer : public Peer {
 public:
   chord::ChordFacade* get_chord() const { return chord.get(); }
   chord::fs::Facade* get_filesystem() const { return filesystem.get(); };
   const chord::Context& get_context() const { return context; };

   IntPeer(const Context& context) : Peer(context) {}
};

}  // namespace chord
