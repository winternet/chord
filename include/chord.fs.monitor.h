#pragma once

#include <thread>
#include <ctime>
#include <optional>
#include <map>
#include <vector>
#include <functional>
#include <libfswatch/c++/monitor_factory.hpp>

#include "chord.signal.h"
#include "chord.path.h"

namespace chord { struct Context; }

namespace chord {
namespace fs {

class monitor {

public:
  struct event {

    struct flag {
      private:
        const fsw_event_flag _flag;
        const char* _string;
        constexpr explicit flag(fsw_event_flag f, const char* str) : _flag{f}, _string{str} {}
      public:
        inline std::string name() const { return _string; }
        inline fsw_event_flag mapped_type() const { return _flag; }
        inline bool operator==(const flag& other) const { return _flag == other._flag; }
        inline bool operator!=(const flag& other) const { return _flag != other._flag; }

        static const flag CREATED;
        static const flag UPDATED;
        static const flag REMOVED;

        static std::vector<flag> values() {
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
      int counter{1};
      std::optional<event::flag> flag;

      filter(const chord::path& path) : path{path} {}
      filter(const chord::path& path, int counter) : path{path}, counter{counter}, flag{} {}
      filter(const chord::path& path, int counter, const event::flag& flag) : path{path}, counter{counter}, flag{flag} {}

      filter(const filter&) = default;
      filter& operator=(filter other) {
        std::swap(path, other.path);
        std::swap(counter, other.counter);
        if(other.flag) flag.emplace(*other.flag);
        return *this;
      }
    };

    std::string path;
    std::vector<flag> flags;
    std::time_t time;

    friend std::ostream& operator<<(std::ostream&, const event&);
  };

  using event_t = signal<void(const std::vector<event>)>;

private:
  fsw::monitor* mon = nullptr;

  std::thread thrd;
  event_t fs_events;
  std::vector<event::filter> _filters;

  void process(const std::vector<fsw::event>& events) {
    //for(const auto& m:events) {
    //  std::string flags;
    //    for(const auto& f:m.get_flags()) {
    //      flags += m.get_event_flag_name(f);
    //    }
    //    std::cout << "\n>" << m.get_time() << " [" << flags << "] -> " << m.get_path();
    //}

    std::vector<event> mapped_events;
    std::transform(events.begin(), events.end(), std::back_inserter(mapped_events), [](const auto& ev) { 
        event mapped;
        mapped.path = ev.get_path();
        mapped.time = ev.get_time();
        const auto flags = ev.get_flags();
        std::transform(flags.begin(), flags.end(), std::back_inserter(mapped.flags), [](const auto& fl) {
            return *event::flag::from(fl);
        });
        return mapped;
      });
    const auto rem_begin = std::remove_if(mapped_events.begin(), mapped_events.end(), [&](const auto& e) {
        return std::any_of(_filters.begin(), _filters.end(), [&](auto& f) {
            const bool filter_has_flag = f.flag.has_value();
            bool flag_matches = filter_has_flag;
            if(flag_matches) {
              const auto event_flags = e.flags;
              flag_matches = std::any_of(event_flags.cbegin(), event_flags.cend(), [&](const auto& fi) { return fi == *f.flag; });
            }
            bool filter_matches = f.path == e.path && (!filter_has_flag || flag_matches);
            return filter_matches && f.counter-- > 0;
        });
    });

    // cleanup filters
    _filters.erase(std::remove_if(_filters.begin(), _filters.end(), [](const auto& f) {return f.counter <= 0;}), _filters.end());
    // remove filtered events
    mapped_events.erase(rem_begin, mapped_events.end());
    // signal events
    if(!mapped_events.empty()) {
      fs_events(mapped_events);
    }
  }

public:
  monitor(const chord::Context&);
  ~monitor();

  monitor(const monitor&) = delete;
  monitor& operator=(const monitor&) = delete;

  event_t& events() {
    return fs_events;
  }

  void stop() const;
  void add_filter(const event::filter&);
  std::vector<event::filter> filters() const;

  friend std::ostream& operator<<(std::ostream&, const monitor::event&);

};

} //namespace fs
} //namespace chord

std::ostream& operator<<(std::ostream&, const chord::fs::monitor::event::flag&);
