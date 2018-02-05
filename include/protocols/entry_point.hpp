#ifndef ENTRY_POINT_HPP
#define ENTRY_POINT_HPP

#include "byte_array/referenced_byte_array.hpp"

namespace fetch 
{
namespace protocols 
{
  
struct EntryPoint  
{
  std::string address;
  uint32_t shard = 0;
  uint32_t port = 1337; 
};
};

namespace serializers 
{
template< typename T >
T& Serialize(T& serializer, protocols::EntryPoint const &data) 
{
  serializer << data.address;
  serializer << data.shard;
  serializer << data.port;
  return serializer;
}

template< typename T >
T& Deserialize(T& serializer, protocols::EntryPoint &data) 
{
  serializer >> data.address;
  serializer >> data.shard;
  serializer >> data.port;
  return serializer;
}
  

};
};

#endif  // ENTRY_POINT_HPP
