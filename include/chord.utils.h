#pragma once

#include <map>
#include <string>
#include <functional>
#include "chord.types.h"

namespace grpc {
  class Status;
}

namespace chord {
namespace utils {

  std::string to_string(const grpc::Status& status);

  /**
   * merge two maps with lambda for conflict resolution
   */
  template<class Key, class Value>
  std::map<Key, Value> merge(const std::map<Key,Value>&a, const std::map<Key,Value>&b, std::function<Value(const Value,const Value)> resolver) {
    std::map<Key, Value> retVal;
    for(const auto [key,val] : a) {
      if(b.find(key) != b.end()) {
        retVal[key] = resolver(val, b.at(key));
      }
      retVal[key] = val;
    }
    for(const auto [key,val] : b) {
      if(retVal.find(key) == retVal.end()) {
        retVal[key] = val;
      }
    }
    return retVal;
  }

}  // namespace utils
}  // namespace chord
