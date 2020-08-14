#include "chord.fs.monitor.h"

#include "chord.context.h"

#include <thread>
#include <chrono>
#include <libfswatch/c/cfilter.h>

namespace chord {
namespace fs {

//const monitor::event::flag monitor::event::flag::CREATED = flag(fsw_event_flag::Created, "created");
const monitor::event::flag monitor::event::flag::UPDATED = flag(fsw_event_flag::Updated, "updated");
const monitor::event::flag monitor::event::flag::REMOVED = flag(fsw_event_flag::Removed, "removed");

monitor::monitor(const chord::Context& context)
  : _monitor{fsw::monitor_factory::create_monitor(fsw_monitor_type::system_default_monitor_type,
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


std::ostream& operator<<(std::ostream& os, const monitor::event& event) {
  os << event.time << " |";
  for(const auto& f:event.flags) {
    os << f.name();
  }
  return os << "| -> " << event.path;
}

} //namespace fs
} //namespace chord


std::ostream& operator<<(std::ostream&os, const chord::fs::monitor::event::flag& flag) {
  return os << flag.name();
}

