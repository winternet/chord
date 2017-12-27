#pragma once

#include <string>

#include "chord.router.h"

endpoint_t peer_to_address(const std::string &peer, int server_port) {
  const char delim = ':';
  size_t beg = peer.find(delim);
  size_t end = peer.rfind(delim) + 1;

  return peer.substr(beg, end - beg) + delim + std::to_string(server_port);
}
