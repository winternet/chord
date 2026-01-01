#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>
#include <spdlog/logger.h>
#include "chord.context.h"
#include "chord.upnp.h"
#include "chord.utils.h"

namespace chord{
  namespace upnp{
void setup_upnp_port_forwarding(const chord::Context& context) {
  const auto logger = context.logging.factory().get_or_create(chord::upnp::logger_name);
  const auto port = chord::utils::port(context);
  logger->info("[upnp] trying to setup upnp port forwarding (:{}).", port);

  int ret;
  UPNPDev *devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, 2, &ret);

  if(ret != UPNPCOMMAND_SUCCESS) {
      logger->warn("[upnp] failed to discover: {}", strupnperror(ret));
  } else {
    logger->trace("[upnp] device list {}", devlist->st);
  }
  UPNPUrls urls;
  IGDdatas data;
  char lanaddr[64];

  ret = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
  if(ret != 0) {
    std::string desc = "chord-p2p";
    int ret = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype, 
                                  port.c_str(), port.c_str(), lanaddr, 
                                  desc.c_str(), "TCP", nullptr, nullptr);

    if(ret == UPNPCOMMAND_SUCCESS){
      logger->info("[upnp] successfully mapped {} via UPnP: internal={}, external={}", port, lanaddr, devlist->descURL);
    } else {
      logger->warn("[upnp] failed to map port {} via UPnP({}): {}", port, ret, strupnperror(ret));
    }
  } else {
      logger->warn("[upnp] failed to map port {} via UPnP({}): {}", port, ret, strupnperror(ret));
  }

  freeUPNPDevlist(devlist);
  FreeUPNPUrls(&urls);
}
  }
}
