#pragma once
#include <utility>

namespace chord {

template<class T>
class Singleton {
protected:
  Singleton() = default;
  ~Singleton() = default;
public:
  template<class... Args>
  static T& instance(Args... args) {
    static T instance(std::forward<Args>(args)...);
    return instance;
  }

  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton) = delete;
};

} //namespace chord

