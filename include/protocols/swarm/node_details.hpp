#ifndef NODE_DETAILS_HPP
#define NODE_DETAILS_HPP

#include "byte_array/referenced_byte_array.hpp"
#include "protocols/swarm/entry_point.hpp"
#include "mutex.hpp"

namespace fetch 
{

namespace protocols 
{
  
struct NodeDetails 
{
  bool operator==(NodeDetails const& other) 
  {
    return public_key == other.public_key;
  }

  byte_array::ByteArray public_key;
  std::vector< EntryPoint > entry_points;
  
  uint32_t default_port;
  uint32_t default_http_port;
}; 

class SharedNodeDetails
{
public:
  SharedNodeDetails() { }
  SharedNodeDetails(SharedNodeDetails const&) = delete;
   
  bool operator==(SharedNodeDetails const& other) 
  {
    return details_.public_key == other.details_.public_key;
  }

  void AddEntryPoint(EntryPoint const& ep) 
  {

    mutex_.lock();    
    bool found_ep = false;
    for(auto &e:   details_.entry_points)
    {
      if( (e.host == ep.host) &&
          (e.port == ep.port))
      {
        found_ep = true;
        break;          
      }
    }
    if(!found_ep) details_.entry_points.push_back( ep );
    mutex_.unlock();    
  }

  uint32_t default_port()
  {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );
    return details_.default_port;    
  }
  
  uint32_t default_http_port() 
  {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );
    return details_.default_http_port;    
  }
  
  
  void with_details(std::function< void(NodeDetails &) > fnc ) 
  {
    mutex_.lock();
    fnc( details_ );
    mutex_.unlock();    
  }

  
  NodeDetails details() {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );

    return details_;    
  }
private:
  NodeDetails details_ ; 
  fetch::mutex::Mutex mutex_;
};

  
  
};
};

#endif 
