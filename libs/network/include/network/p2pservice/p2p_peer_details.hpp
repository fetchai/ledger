#pragma once
#include"core/byte_array/byte_array.hpp"
#include"crypto/fnv.hpp"
#include"crypto/identity.hpp"
#include"crypto/prover.hpp"
#include"crypto/verifier.hpp"
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
    was_promoted = false;
  }
  
  EntryPoint(EntryPoint const&other) 
  {
    for(auto &s:other.host)
    {
      host.insert(s.Copy());
    }
    
    port = other.port;
    identity = other.identity;    
    lane_id = uint32_t(other.lane_id);
    is_discovery = bool(other.is_discovery);    
    is_lane = bool(other.is_lane);
    is_mainchain = bool(other.is_mainchain);
    was_promoted = bool(other.was_promoted);    
  }

  EntryPoint& operator=(EntryPoint const&other) 
  {
    for(auto &s:other.host)
    {
      host.insert(s.Copy());
    }
    
    port = other.port;
    identity = other.identity;
    lane_id = uint32_t(other.lane_id);
    is_lane = bool(other.is_lane);
    is_mainchain = bool(other.is_mainchain);
    was_promoted = bool(other.was_promoted);        
    return *this;
  }
  
  
  /// Serializable fields
  /// @{
  std::unordered_set< byte_array::ConstByteArray, crypto::CallableFNV > host;
  uint16_t port = 0;

  crypto::Identity identity;  
  std::atomic< uint32_t > lane_id;

  std::atomic< bool > is_discovery;  
  std::atomic< bool > is_lane;
  std::atomic< bool > is_mainchain;  
  /// @}

  std::atomic< bool > was_promoted;
  
};

template <typename T>
T& Serialize(T& serializer, EntryPoint const& data) {
  serializer << data.host;
  serializer << data.port;
  serializer << data.identity;
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
  serializer >> data.identity;
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
    identity = other.identity;
    entry_points = other.entry_points;

    nonce = other.nonce;
    karma = double(other.karma);
    is_authenticated = bool(other.is_authenticated);
    last_updated = std::chrono::system_clock::now();    
  }

  PeerDetails& operator=(PeerDetails const & other)  
  {
    identity = other.identity;    
    entry_points = other.entry_points;

    nonce = other.nonce;
    karma = double(other.karma);
    is_authenticated = bool(other.is_authenticated);
    last_updated = std::chrono::system_clock::now();
    return *this;
  }

  void Update(PeerDetails const & other)  
  {
    identity = other.identity;  
    entry_points = other.entry_points;
  }

  double MillisecondsSinceUpdate() const 
  {
    std::chrono::system_clock::time_point end =
      std::chrono::system_clock::now();
    double ms = double(std::chrono::duration_cast<std::chrono::milliseconds>(end - last_updated).count());
    return ms;
  }

  void Sign(crypto::Prover *prov) 
  {
    serializers::ByteArrayBuffer buffer;
    // TODO: Count count first
    buffer << identity << entry_points;    
    prov->Sign(buffer.data());
    signature = prov->signature();
  }

  bool Verify(crypto::Verifier *ver) 
  {
    
    serializers::ByteArrayBuffer buffer;
    // TODO: Count count first
    buffer << identity << entry_points;    
    return ver->Verify(buffer.data(), signature);
  }

  
  
  /// Serializable
  /// @{
  crypto::Identity identity;
  std::vector< EntryPoint > entry_points;
  byte_array::ConstByteArray signature;
  /// @}
  

  /// Peer meta data
  /// @{  
  byte_array::ConstByteArray nonce;
  std::atomic< double > karma;
  std::atomic< bool > is_authenticated;
  std::chrono::system_clock::time_point last_updated;
  /// @}
};

template <typename T>
T& Serialize(T& serializer, PeerDetails const& data) {
  serializer << data.identity;
  serializer << data.entry_points;

  return serializer;
}

template <typename T>
T& Deserialize(T& serializer, PeerDetails& data) {
  serializer >> data.identity;
  serializer >> data.entry_points;

  
  return serializer;
}



}
}
