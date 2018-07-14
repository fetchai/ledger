#ifndef NETWORK_P2PSERVICE_P2P_PEER_DETAILS_HPP
#define NETWORK_P2PSERVICE_P2P_PEER_DETAILS_HPP
#include"core/byte_array/byte_array.hpp"

#include<atomic>

namespace fetch
{
namespace p2p
{

struct EntryPoint : public std::enable_shared_from_this< EntryPoint >
{
  mutable mutex::Mutex lock;

  /// Serializable fields
  /// @{
  byte_array::ConstByteArray host;
  uint16_t port;

  byte_array::ConstByteArray public_key;

  std::atomic< uint64_t > lane_id;  
  /// @}

  /// Meta data for keeping track of things
  /// @{
  std::atomic< bool > is_lane;
  std::atomic< bool > is_mainchain;
  /// @}

  
};

template <typename T>
T& Serialize(T& serializer, EntryPoint const& data) {
  serializer << data.host;
  serializer << data.port;
  serializer << data.public_key;
  serializer << uint64_t( data.lane_id);

  return serializer;
}

template <typename T>
T& Deserialize(T& serializer, EntryPoint& data) {
  serializer >> data.host;
  serializer >> data.port;
  serializer >> data.public_key;
  uint64_t lane;
  
  serializer >> lane;
  data.lane_id = lane;
  
  return serializer;
}

 
struct PeerDetails
{
  PeerDetails() 
  {
    karma = 0;
  }
  
  /// Serializable
  /// @{
  byte_array::ConstByteArray public_key;
  std::vector< EntryPoint > entry_points;
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
