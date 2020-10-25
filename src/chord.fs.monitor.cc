#include "chord.fs.monitor.h"

#include "chord.context.h"

#include <thread>
#include <chrono>
#include <libfswatch/c/cfilter.h>

namespace chord {
namespace fs {

const monitor::event::flag monitor::event::flag::CREATED = flag(fsw_event_flag::Created, "created");
const monitor::event::flag monitor::event::flag::UPDATED = flag(fsw_event_flag::Updated, "updated");
const monitor::event::flag monitor::event::flag::REMOVED = flag(fsw_event_flag::Removed, "removed");

monitor::monitor(const chord::Context& context)
  : _monitor{fsw::monitor_factory::create_monitor(fsw_monitor_type::poll_monitor_type,
      {context.data_directory},
      [](const std::vector<fsw::event>& events, void* context) {
        auto monitor = static_cast<class monitor*>(context);
        monitor->process(events);
      })} 
{
  _monitor->set_context(this);
  _monitor->set_recursive(true);
  _monitor->set_follow_symlinks(true);
  _monitor->set_fire_idle_event(false);
  for(const auto& flag:event::flag::values()) {
    _monitor->add_event_type_filter(fsw_event_type_filter{flag.mapped_type()});
  }

  _thread = std::thread{&fsw::monitor::start,std::ref(_monitor)};
}

monitor::~monitor() {
  stop();
  _scheduler.shutdown();
  _thread.join();
  if(_monitor) delete _monitor;
}

void monitor::stop() const {
  if(_monitor) _monitor->stop();
}

void monitor::add_filter(const event::filter& f) {
  _filters.push_back(f);
  _filters_cow.push_back(f);
}

void monitor::remove_filter(const event::filter& f) {
  //TODO check whether removing the first matching filter makes sense in all cases 
  //     or whether the filter needs to be more specific (e.g. uuid)
  const auto iter = std::find(_filters.begin(), _filters.end(), f);
  if(iter != _filters.end()) {
    _filters.erase(iter);
  }
}

std::vector<monitor::event::filter> monitor::filters() const {
  return _filters;
}

void monitor::process(std::vector<fsw::event> events) {
  //for(const auto& m:events) {
  //  std::string flags;
  //    for(const auto& f:m.get_flags()) {
  //      flags += m.get_event_flag_name(f);
  //    }
  //    std::cout << "\n> " << m.get_time() << " [" << flags << "] -> " << m.get_path();
  //}
  //-----
  std::vector<event> mapped_events;

  // map to monitor::event
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

  // erase directory events
  mapped_events.erase(std::remove_if(mapped_events.begin(), mapped_events.end(), [](const event& e) { return chord::file::is_directory(e.path); }), mapped_events.end());

  // erase by filter
  const auto rem_begin = std::remove_if(mapped_events.begin(), mapped_events.end(), [&](const auto& e) {
      return std::any_of(_filters_cow.begin(), _filters_cow.end(), [&](auto& f) {
          const bool filter_has_flag = f.flag.has_value();
          bool flag_matches = filter_has_flag;
          if(flag_matches) {
            const auto event_flags = e.flags;
            flag_matches = std::any_of(event_flags.cbegin(), event_flags.cend(), [&](const auto& fi) { return fi == *f.flag; });
          }
          bool filter_matches = f.path == e.path && (!filter_has_flag || flag_matches);
          return filter_matches;
      });
  });

  _filters_cow = _filters;

  // cleanup filters
  //_filters.erase(std::remove_if(_filters.begin(), _filters.end(), [](const auto& f) {return f.counter <= 0;}), _filters.end());
  // remove filtered events
  mapped_events.erase(rem_begin, mapped_events.end());

  {
    std::scoped_lock<std::mutex> lock(_mutex);
    _events.insert(_events.end(), mapped_events.begin(), mapped_events.end());
  }

  // signal events
  if(!mapped_events.empty()) {
    fire_events();
  }

}

void monitor::fire_events() {
  std::scoped_lock<std::mutex> lock(_mutex);
  if(!_events.empty()) {

    // most recent events first
    std::reverse(_events.begin(), _events.end());

    // debounce
    std::sort(_events.begin(), _events.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.path < rhs.path;
    });
    // unique keeps first (most recent) event
    _events.erase(std::unique(_events.begin(), _events.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.path == rhs.path;
    }), _events.end());

    fs_events(_events);
    _events.clear();
  }
}

std::ostream& operator<<(std::ostream& os, const monitor::event& event) {
  os << event.time << " |";
  for(const auto& f:event.flags) {
    os << f.name();
  }
  return os << "| -> " << event.path;
}

std::ostream& operator<<(std::ostream&os, const chord::fs::monitor::event::flag& flag) {
  return os << flag.name();
}

} //namespace fs
} //namespace chord



