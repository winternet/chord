#pragma once

#include <thread>
#include <ctime>
#include <optional>
#include <iterator>
#include <map>
#include <vector>
#include <functional>
#include <libfswatch/c++/monitor_factory.hpp>

#include "chord.signal.h"
#include "chord.scheduler.h"
#include "chord.path.h"
#include "chord.file.h"

namespace chord { struct Context; }

namespace chord {
namespace fs {

class monitor {

public:
  struct event {

    struct flag {
      private:
        fsw_event_flag _flag;
        std::string _string;
        explicit flag(fsw_event_flag f, const std::string& str) : _flag{f}, _string{str} {}

      public:
        flag(const flag& other) : _flag{other._flag}, _string{other._string} {}
        flag& operator=(const flag& other) {
          _flag = other._flag;
          _string = other._string;
          return *this;
        }
        ~flag() = default;

        inline std::string name() const { return _string; }
        inline fsw_event_flag mapped_type() const { return _flag; }
        inline bool operator==(const flag& other) const { return _flag == other._flag; }
        inline bool operator!=(const flag& other) const { return _flag != other._flag; }

        static const flag CREATED;
        static const flag UPDATED;
        static const flag REMOVED;

        static std::vector<flag> values() {
          //return {/*CREATED,*/ UPDATED, REMOVED};
          return {CREATED, UPDATED, REMOVED};
        }

        static std::optional<flag> from(const fsw_event_flag& f) {
          for(const auto& mapped_type : values()) {
            if(mapped_type._flag == f) {
              return mapped_type;
            }
          }
          return {};
        }
    };

    struct filter {
      chord::path path;
      std::optional<event::flag> flag;

      filter(const chord::path& path) : path{path} {}
      filter(const chord::path& path, const event::flag& flag): path{path}, flag{flag} {}

      filter(const filter&) = default;
      filter& operator=(filter other) {
        std::swap(path, other.path);
        if(other.flag) flag.emplace(*other.flag);
        return *this;
      }
      bool operator==(const filter& other) const {
        return path == other.path; // && flag == other.flag && counter == other.flag;
      }
      bool operator!=(const filter& other) const {
        return !(*this == other);
      }
    };

    std::string path;
    std::vector<flag> flags;
    std::time_t time;

    friend std::ostream& operator<<(std::ostream&, const event&);
  };

  struct lock {
    monitor* _monitor;
    event::filter _filter;

    lock(const lock&) = delete;
    lock& operator=(const lock&) = delete;

    lock(monitor* mon, const event::filter& fil) : _monitor{mon}, _filter{fil} {
      if(_monitor) _monitor->add_filter(_filter);
    }
    ~lock() {
      if(_monitor) _monitor->remove_filter(_filter);
    }
  };

  using event_t = signal<void(std::vector<event>)>;

private:
  fsw::monitor* _monitor = nullptr;

  std::mutex _mutex;
  Scheduler _scheduler;

  std::thread _thread;
  event_t fs_events;

  std::vector<event::filter> _filters;
  std::vector<event> _events;
  std::vector<event::filter> _filters_cow;

  void process(std::vector<fsw::event> events);
  void fire_events();

public:
  monitor(const chord::Context&);
  virtual ~monitor();

  monitor(const monitor&) = delete;
  monitor& operator=(const monitor&) = delete;

  event_t& events() {
    return fs_events;
  }

  void stop() const;
  void add_filter(const event::filter&);
  void remove_filter(const event::filter&);
  std::vector<event::filter> filters() const;

  friend std::ostream& operator<<(std::ostream&, const monitor::event&);

};

std::ostream& operator<<(std::ostream&, const chord::fs::monitor::event::flag&);

} //namespace fs
} //namespace chord


