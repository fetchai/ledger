#ifndef ENTRY_POINT_HPP
#define ENTRY_POINT_HPP

#include "byte_array/referenced_byte_array.hpp"

namespace fetch 
{
namespace protocols 
{

struct EntryPoint  
{
  std::string host;
  uint32_t shard = 0;
  uint32_t port = 1337;
  uint32_t http_port = 8080;
  uint16_t configuration = 0;  
};
};

namespace serializers 
{
template< typename T >
T& Serialize(T& serializer, protocols::EntryPoint const &data) 
{
  serializer << data.host;
  serializer << data.shard;
  serializer << data.port;
  serializer << data.http_port;
  serializer << data.configuration;
  return serializer;
}

template< typename T >
T& Deserialize(T& serializer, protocols::EntryPoint &data) 
{
  serializer >> data.host;
  serializer >> data.shard;
  serializer >> data.port;
  serializer >> data.http_port;
  serializer >> data.configuration;  
  return serializer;
}
  

};
};

#endif  // ENTRY_POINT_HPP
