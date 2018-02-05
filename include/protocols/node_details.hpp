#ifndef NODE_DETAILS_HPP
#define NODE_DETAILS_HPP

#include "byte_array/referenced_byte_array.hpp"
#include "protocols/entry_point.hpp"

namespace fetch 
{
  namespace protocols // fetch::protocols
{
  
struct NodeDetails 
{
  byte_array::ByteArray public_key;
  std::vector< EntryPoint > entry_points;

  bool operator==(NodeDetails const& other) 
  {
    return public_key == other.public_key;
  }
}; // namespace protocols
};

namespace serializers // fetch::serializers
{
  
template< typename T >
T& Serialize(T& serializer, protocols::NodeDetails const &data) 
{
  serializer << data.public_key;
  serializer << uint64_t(data.entry_points.size());
  for(auto const &e: data.entry_points) 
  {
    serializer << e;
  }
  return serializer;
}

template< typename T >
T& Deserialize(T& serializer, protocols::NodeDetails &data) 
{
  serializer >> data.public_key;
  uint64_t size;
  serializer >> size;
  data.entry_points.resize(size);
  for(auto &e: data.entry_points) 
  {
    serializer >> e;
  }
  return serializer;
}
  
template< typename T >
T& Serialize(T& serializer, std::vector< protocols::NodeDetails > const &data) 
{
  serializer << uint64_t(data.size() );
  for(auto const &e: data) 
  {
    serializer << e;
  }
  return serializer;
}
  
template< typename T >
T& Deserialize(T& serializer, std::vector< protocols::NodeDetails >  &data) 
{
  uint64_t size;
  serializer >> size;
  data.resize(size);
  for(auto &e: data) 
  {
    serializer >> e;
  }
  return serializer;
}
  
}; // namespace fetch::serializers
}; // namespace fetch

#endif // NODE_DETAILS_HPP
