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
//    mutex_.lock();    
    bool found_ep = false;
    for(auto &e:   entry_points_)
    {
      if( (e.host == ep.host) &&
          (e.port == ep.port))
      {
        found_ep = true;
        break;          
      }
    }
    if(!found_ep) entry_points_.push_back( ep );
//    mutex_.unlock();    
  }
  
  
  byte_array::ByteArray const &public_key() const { return public_key_; }
  std::vector< EntryPoint > const &entry_points() const { return entry_points_; }
  
  byte_array::ByteArray  &public_key() { return public_key_; }
  std::vector< EntryPoint >  &entry_points() { return entry_points_; }

  uint32_t const & default_port() const { return default_port_; }
  uint32_t &default_port() { return default_port_; }  

  uint32_t const & default_http_port() const { return default_http_port_;  }
  uint32_t &default_http_port() { return default_http_port_;  }  

  void with_this(std::function< void(NodeDetails const &) > fnc ) 
  {
//    mutex_.lock();
    fnc( *this );
//    mutex_.unlock();    
  }
  
private:
  byte_array::ByteArray public_key_;
  std::vector< EntryPoint > entry_points_;
  
  uint32_t default_port_;
  uint32_t default_http_port_;


//  fetch::mutex::Mutex mutex_;
}; 

  
  
};
};

#endif 
