#pragma once

#include <set>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include "chord.uri.h"
#include "chord.path.h"
#include "chord.types.h"
#include "chord.context.h"

namespace grpc {
  class Status;
}

namespace chord {
namespace utils {

  uri as_uri(const std::string&);
  uri as_uri(const char*);
  uri as_uri(const path&);

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

  template <class T, class Comp, class Alloc, class Predicate>
  void erase_if(std::set<T, Comp, Alloc>& s, Predicate pred) {
    for(auto it=s.begin(); it != s.end();)
      if(pred(*it)) it = s.erase(it);
      else ++it;
  }

  /**
   * parsing program options
   */
  struct Options {

    Context context;

    // controller
    endpoint address;
    std::vector<std::string> commands;
  };

  Options parse_program_options(int ac, char *av[]);

}  // namespace utils
}  // namespace chord
