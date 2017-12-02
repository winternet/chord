#pragma once

#include <memory>
#include <functional>

#include "chord.log.h"
#include "chord.uuid.h"
#include "chord.router.h"
#include "chord_controller.grpc.pb.h"


typedef std::function<std::shared_ptr<chord::controller::Control::StubInterface>(const endpoint_t& endpoint)> ControlStubFactory;

namespace chord {
  namespace controller {
    class Client {
      private:
        ControlStubFactory make_stub;

        /**
         * return new header
         */
        //Header make_header();
        //SuccessorRequest make_request();

      public:
        Client();
        Client(ControlStubFactory factory);

        void control(const std::string& command);

    };
  } //namespace controller
} //namespace chord
