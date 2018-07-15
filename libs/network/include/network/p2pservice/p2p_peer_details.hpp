#ifndef NETWORK_P2PSERVICE_P2P_PEER_DETAILS_HPP
#define NETWORK_P2PSERVICE_P2P_PEER_DETAILS_HPP
#include"core/byte_array/byte_array.hpp"
#include"crypto/fnv.hpp"

#include<atomic>

namespace fetch
{
namespace p2p
{

struct EntryPoint 
{
  mutable mutex::Mutex lock;

  EntryPoint() 
  {
    port = 0;
    lane_id = uint32_t(-1);
    is_discovery = false;
    is_lane = false;
    is_mainchain = false;
  }
  
  EntryPoint(EntryPoint const&other) 
  {
    for(auto &s:other.host)
    {
      host.insert(s.Copy());
    }
    
    port = other.port;
    lane_id = uint32_t(other.lane_id);
    is_discovery = bool(other.is_discovery);    
    is_lane = bool(other.is_lane);
    is_mainchain = bool(other.is_mainchain);
  }

  EntryPoint& operator=(EntryPoint const&other) 
  {
    for(auto &s:other.host)
    {
      host.insert(s.Copy());
    }
    
    port = other.port;
    lane_id = uint32_t(other.lane_id);
    is_lane = bool(other.is_lane);
    is_mainchain = bool(other.is_mainchain);
    return *this;
  }
  
  
  /// Serializable fields
  /// @{
  std::unordered_set< byte_array::ConstByteArray, crypto::CallableFNV > host;
  uint16_t port = 0;

  byte_array::ConstByteArray public_key;

  std::atomic< uint32_t > lane_id;  

  std::atomic< bool > is_discovery;  
  std::atomic< bool > is_lane;
  std::atomic< bool > is_mainchain;
  /// @}

  
};

template <typename T>
T& Serialize(T& serializer, EntryPoint const& data) {
  serializer << data.host;
  serializer << data.port;
  serializer << data.public_key;
  serializer << uint32_t( data.lane_id);
  serializer << bool(data.is_discovery);  
  serializer << bool(data.is_lane);
  serializer << bool(data.is_mainchain);  
  return serializer;
}

template <typename T>
T& Deserialize(T& serializer, EntryPoint& data) {
  serializer >> data.host;
  serializer >> data.port;
  serializer >> data.public_key;
  uint32_t lane;
  
  serializer >> lane;
  data.lane_id = lane;

  bool islane, ismc, isdisc;
  serializer >> isdisc;
  data.is_discovery = isdisc;
  
  serializer >> islane;
  data.is_lane = islane;
  
  serializer >> ismc;
  data.is_mainchain = ismc;
  
  return serializer;
}

 
struct PeerDetails
{
  PeerDetails() 
  {
    karma = 0;
    is_authenticated = false;
  }

  PeerDetails(PeerDetails const & other)  
  {
    public_key = other.public_key.Copy();
    entry_points = other.entry_points;

    // TODO: consider whether to reset these fields 
    nonce = other.nonce;
    karma = double(other.karma);
    is_authenticated = bool(other.is_authenticated);
  }

  PeerDetails& operator=(PeerDetails const & other)  
  {
    public_key = other.public_key.Copy();
    entry_points = other.entry_points;

    // TODO: consider whether to reset these fields 
    nonce = other.nonce;
    karma = double(other.karma);
    is_authenticated = bool(other.is_authenticated);
    return *this;
  }

  void Update(PeerDetails const & other)  
  {
    public_key = other.public_key.Copy();
    entry_points = other.entry_points;
    looking_for_peers_timeout = uint32_t(other.looking_for_peers_timeout);
    
  }
  
  /// Serializable
  /// @{
  byte_array::ConstByteArray public_key;
  std::vector< EntryPoint > entry_points;
  std::atomic< uint32_t > looking_for_peers_timeout;
  
  /// @}

  /// Peer meta data
  /// @{  
  byte_array::ConstByteArray nonce;
  std::atomic< double > karma;
  std::atomic< bool > is_authenticated;

  /// @}
};

template <typename T>
T& Serialize(T& serializer, PeerDetails const& data) {
  serializer << data.public_key;
  serializer << data.entry_points;

  return serializer;
}

template <typename T>
T& Deserialize(T& serializer, PeerDetails& data) {
  serializer >> data.public_key;
  serializer >> data.entry_points;

  
  return serializer;
}



}
}
#endif
