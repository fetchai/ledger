#ifndef NODE_DETAILS_HPP
#define NODE_DETAILS_HPP

#include "byte_array/referenced_byte_array.hpp"
#include "protocols/swarm/entry_point.hpp"

namespace fetch 
{

namespace protocols 
{
  
class NodeDetails 
{
public:
  NodeDetails() 
  {
  }
  
  bool operator==(NodeDetails const& other) 
  {
    return public_key_ == other.public_key_;
  }

  void AddEntryPoint(EntryPoint const& ep) 
  {
    // TODO: Add mutex
    bool found_ep = false;
    for(auto &e:   details_.entry_points())
    {
      if( (e.host == ep.host) &&
          (e.port == ep.port))
      {
        found_ep = true;
        break;          
      }
    }
    if(!found_ep) entry_points_.push_back( ep );    
  }
  
  
  byte_array::ByteArray const &public_key() const { return public_key_; }
  std::vector< EntryPoint > const &entry_points() const { return entry_points_; }
  
  byte_array::ByteArray  &public_key() { return public_key_; }
  std::vector< EntryPoint >  &entry_points() { return entry_points_; }

  uint32_t const & default_port() const { return default_port_; }
  uint32_t &default_port() { return default_port_; }  

  uint32_t const & default_http_port() const { return default_http_port_;  }
  uint32_t &default_http_port() { return default_http_port_;  }  
  
private:
  byte_array::ByteArray public_key_;
  std::vector< EntryPoint > entry_points_;

  uint32_t default_port_;
  uint32_t default_http_port_;  

}; 
};

namespace serializers
{
  
template< typename T >
T& Serialize(T& serializer, protocols::NodeDetails const &data) 
{
  serializer << data.public_key();
  serializer << uint64_t(data.entry_points().size());
  for(auto const &e: data.entry_points()) 
  {
    serializer << e;
  }
  return serializer;
}

template< typename T >
T& Deserialize(T& serializer, protocols::NodeDetails &data) 
{
  serializer >> data.public_key();
  uint64_t size;
  serializer >> size;
  data.entry_points().resize(size);
  for(auto &e: data.entry_points()) 
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
  
};
};

#endif 
