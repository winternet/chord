#pragma once

namespace chord {

template<class T>
class Singleton {
protected:
  Singleton() {}
  ~Singleton() {}
public:
  static T& instance() {
    static T instance;
    return instance;
  }

  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton) = delete;
};

}

