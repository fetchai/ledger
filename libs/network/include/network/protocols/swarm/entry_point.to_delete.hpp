#pragma once

#include "byte_array/referenced_byte_array.hpp"
#include "chain/transaction.hpp"

namespace fetch {
namespace protocols {

struct EntryPoint
{
  enum
  {
    NODE_SWARM        = 1ull << 16,
    NODE_CHAIN_KEEPER = 2ull << 16
  };

  enum
  {
    IP_UNKNOWN = 1ull << 15
  };

  std::string host          = "";
  group_type  group         = 0;
  uint16_t    port          = 1337;
  uint16_t    http_port     = 8080;
  uint64_t    configuration = 0;

  bool operator==(EntryPoint const &other)
  {
    return (group == other.group) && (port == other.port) && (http_port == other.http_port) &&
           (configuration == other.configuration) && (host == other.host);
  }

  bool operator!=(EntryPoint const &other) { return !(this->operator==(other)); }
};
}  // namespace protocols
}  // namespace fetch
