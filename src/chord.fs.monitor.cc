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
  : mon{fsw::monitor_factory::create_monitor(fsw_monitor_type::system_default_monitor_type,
      {context.data_directory},
      [](const std::vector<fsw::event>& events, void* context) {
        auto monitor = static_cast<class monitor*>(context);
        monitor->process(events);
      })} 
{
  mon->set_context(this);
  mon->set_recursive(true);
  mon->set_follow_symlinks(true);
  mon->set_fire_idle_event(false);
  for(const auto& flag:event::flag::values()) {
    mon->add_event_type_filter(fsw_event_type_filter{flag.mapped_type()});
  }

  thrd = std::thread{&fsw::monitor::start,std::ref(mon)};
}

monitor::~monitor() {
  stop();
  if(mon) delete mon;
  thrd.join();
}

void monitor::stop() const {
  if(mon) mon->stop();
}

void monitor::add_filter(const event::filter& f) {
  _filters.push_back(f);
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

