#ifndef ENTRY_POINT_HPP
#define ENTRY_POINT_HPP

#include "byte_array/referenced_byte_array.hpp"

namespace fetch 
{
namespace protocols 
{

struct EntryPoint  
{
  enum {
    NODE_SWARM = 1ull << 16,
    NODE_SHARD = 2ull << 16 
  };

  enum {
    IP_UNKNOWN = 1ull << 15
  };
  
  
  std::string host = "";
  uint32_t shard = 0;
  uint32_t port = 1337;
  uint32_t http_port = 8080;
  uint64_t configuration = 0;

  bool operator==(EntryPoint const& other) 
  {
    return (shard == other.shard) && (port == other.port) && (http_port == other.http_port) &&      
      (configuration == other.configuration) && (host == other.host);    
  }
  
  bool operator!=(EntryPoint const& other)
  {
    return !(this->operator==(other));    
  }


  
};

};
};

#endif  // ENTRY_POINT_HPP
