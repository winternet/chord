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
   class IntFsFacade : public chord::fs::Facade {
     public:
      IntFsFacade(Context& context, ChordFacade* chord, ChannelPool* channel_pool)
        : chord::fs::Facade(context, chord, channel_pool){}

       chord::fs::IMetadataManager* get_metadata_manager() {
         return chord::fs::Facade::metadata_mgr.get();
       }
   };
 public:
   chord::ChordFacade* get_chord() const { return chord.get(); }
   chord::fs::Facade* get_filesystem() const { return filesystem.get(); };
   const chord::Context& get_context() const { return context; };
   chord::fs::IMetadataManager* get_metadata_manager() const { return static_cast<IntFsFacade*>(filesystem.get())->get_metadata_manager(); }

   IntPeer(Context ctxt) : Peer() {
         this->context = ctxt;
         this->channel_pool = std::make_unique<chord::ChannelPool>(context);
         this->chord = std::make_unique<chord::ChordFacade>(context, channel_pool.get());
         this->filesystem = std::make_unique<IntFsFacade>(context, chord.get(), channel_pool.get());
         this->controller = std::make_unique<controller::Service>(context, filesystem.get());
         init();
     }
};

}  // namespace chord
