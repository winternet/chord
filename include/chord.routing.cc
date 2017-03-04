#include "chord.routing.h"

void init_logs() {
  // formatting   : time [idx] message \n
  // destination  : console
  g_l()->writer().write("%time%($hh:$mm.$ss.$mili) [%idx%] |\n");
  g_l()->mark_as_initialized();
}
