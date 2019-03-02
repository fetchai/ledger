#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <array>
#include <cstdint>
#include <cstring>  // memset
#include <type_traits>

namespace fetch {
namespace muddle {

/**
 * This is the fundamental data structure that is sent around the network
 *
 * It comprises of the fixed size header that is prefixed on top of the variable sized payload.
 * The basic elements of the packet are shown in the diagram below:
 *
 * ┌──────────┬──────────┬──────────┬────────────────────────────────┐
 * │ Version  │  Flags   │   TTL    │            Service             │
 * ├──────────┴──────────┴──────────┼────────────────────────────────┤
 * │            Channel             │            Counter             │
 * ├────────────────────────────────┴────────────────────────────────┤
 * │                           Network Id                            │
 * ├─────────────────────────────────────────────────────────────────┤
 * │                                                                 │
 * │                        From (Public Key)                        │
 * │                                                                 │
 * ├─────────────────────────────────────────────────────────────────┤
 * │                                                                 │
 * │                       Target (Public Key)                       │
 * │                                                                 │
 * ├─────────────────────────────────────────────────────────────────┤
 *
 * │                                                                 │
 *
 * │                         Packet Payload                          │
 *
 * │                                                                 │
 *
 * └ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┘
 */
class Packet
{
public:
  static constexpr std::size_t ADDRESS_SIZE = 64;
  static constexpr std::size_t HEADER_SIZE  = 12 + (2 * ADDRESS_SIZE);

  using RawAddress   = std::array<uint8_t, ADDRESS_SIZE>;
  using Address      = byte_array::ConstByteArray;
  using Payload      = byte_array::ConstByteArray;
  using BinaryHeader = std::array<uint8_t, HEADER_SIZE>;

  struct RoutingHeader
  {
    // clang-format off
    uint32_t version   : 4;   ///< Flag to signal the current version of the muddle protocol
    uint32_t direct    : 1;   ///< Flag to signal that a direct message is being sent (no routing)
    uint32_t broadcast : 1;   ///< Flag to signal that the packet is a broadcast packet
    uint32_t exchange  : 1;   ///< Flag to signal that this is an exchange packet
    uint32_t reserved  : 1;   ///< Reserved for padding
    uint32_t ttl       : 8;   ///< The time to live counter
    uint32_t service   : 16;  ///< The service number (helpful for RPC compatibility)
    uint32_t proto     : 16;  ///< The protocol number (helpful for RPC compatibility)
    uint32_t msg_num   : 16;  ///< Incremented message counter for detecting duplicate packets
    uint32_t network   : 32;  ///< The originating network id
    // clang-format on

    RawAddress target;  ///< The address of the packet target
    RawAddress sender;  ///< The address of the packet sender
  };

  static_assert(std::is_pod<RoutingHeader>::value, "Routing header must be POD");
  static_assert(sizeof(RoutingHeader) == 12 + (2 * ADDRESS_SIZE), "The header must be packed");
  static_assert(sizeof(RoutingHeader) == sizeof(BinaryHeader),
                "The header and binary header must be equivalent");

  // Construction / Destruction
  Packet() = default;
  explicit Packet(Address const &source_address, uint32_t network_id);
  Packet(Packet const &) = delete;
  Packet(Packet &&)      = delete;
  ~Packet()              = default;

  // Operators
  Packet &operator=(Packet const &) = delete;
  Packet &operator=(Packet &&) = delete;

  // Getters
  uint8_t           GetVersion() const;
  bool              IsDirect() const;
  bool              IsBroadcast() const;
  bool              IsExchange() const;
  uint8_t           GetTTL() const;
  uint16_t          GetService() const;
  uint16_t          GetProtocol() const;
  uint16_t          GetMessageNum() const;
  uint32_t          GetNetworkId() const;
  RawAddress const &GetTargetRaw() const;
  RawAddress const &GetSenderRaw() const;
  Address const &   GetTarget() const;
  Address const &   GetSender() const;
  Payload const &   GetPayload() const;

  // Setters
  void SetDirect(bool set = true);
  void SetBroadcast(bool set = true);
  void SetExchange(bool set = true);
  void SetTTL(uint8_t ttl);
  void SetService(uint16_t service_num);
  void SetProtocol(uint16_t protocol_num);
  void SetMessageNum(uint16_t message_num);
  void SetNetworkId(uint32_t network_id);
  void SetTarget(RawAddress const &address);
  void SetTarget(Address const &address);
  void SetPayload(Payload const &payload);

private:
  RoutingHeader header_;   ///< The header containing primarily routing information
  Payload       payload_;  ///< The payload of the message

  ///< Cached versions of the addresses
  mutable Address target_;
  mutable Address sender_;

  template <typename T>
  friend void Serialize(T &serializer, Packet const &b);

  template <typename T>
  friend void Deserialize(T &serializer, Packet &b);
};

inline Packet::Packet(Address const &source_address, uint32_t network_id)
  : header_()
  , payload_()
{
  std::memset(&header_, 0, sizeof(header_));

  // mark the packet with the version
  header_.version = 2;
  header_.network = network_id;

  // add the sender
  assert(source_address.size() == ADDRESS_SIZE);
  std::memcpy(header_.sender.data(), source_address.pointer(), ADDRESS_SIZE);
}

inline uint8_t Packet::GetVersion() const
{
  return static_cast<uint8_t>(header_.version);
}

inline bool Packet::IsDirect() const
{
  return header_.direct > 0;
}

inline bool Packet::IsBroadcast() const
{
  return header_.broadcast > 0;
}

inline bool Packet::IsExchange() const
{
  return header_.exchange > 0;
}

inline uint8_t Packet::GetTTL() const
{
  return static_cast<uint8_t>(header_.ttl);
}

inline uint16_t Packet::GetService() const
{
  return static_cast<uint16_t>(header_.service);
}

inline uint16_t Packet::GetProtocol() const
{
  return static_cast<uint16_t>(header_.proto);
}

inline uint16_t Packet::GetMessageNum() const
{
  return static_cast<uint16_t>(header_.msg_num);
}

inline uint32_t Packet::GetNetworkId() const
{
  return header_.network;
}

inline Packet::RawAddress const &Packet::GetTargetRaw() const
{
  return header_.target;
}

inline Packet::RawAddress const &Packet::GetSenderRaw() const
{
  return header_.sender;
}

inline Packet::Address const &Packet::GetTarget() const
{
  if (target_.size() == 0)
  {
    byte_array::ByteArray target;
    target.Resize(std::size_t{ADDRESS_SIZE});
    std::memcpy(target.pointer(), header_.target.data(), ADDRESS_SIZE);

    target_ = target;
  }

  return target_;
}

inline Packet::Address const &Packet::GetSender() const
{
  if (sender_.size() == 0)
  {
    byte_array::ByteArray sender;
    sender.Resize(std::size_t{ADDRESS_SIZE});
    std::memcpy(sender.pointer(), header_.sender.data(), ADDRESS_SIZE);

    sender_ = sender;
  }

  return sender_;
}

inline Packet::Payload const &Packet::GetPayload() const
{
  return payload_;
}

inline void Packet::SetDirect(bool set)
{
  header_.direct = (set) ? 1 : 0;
}

inline void Packet::SetBroadcast(bool set)
{
  header_.broadcast = (set) ? 1 : 0;
}

inline void Packet::SetExchange(bool set)
{
  header_.exchange = (set) ? 1 : 0;
}

inline void Packet::SetTTL(uint8_t ttl)
{
  header_.ttl = ttl;
}

inline void Packet::SetService(uint16_t service_num)
{
  header_.service = service_num;
}

inline void Packet::SetProtocol(uint16_t protocol_num)
{
  header_.proto = protocol_num;
}

inline void Packet::SetMessageNum(uint16_t message_num)
{
  header_.msg_num = message_num;
}

inline void Packet::SetNetworkId(uint32_t network_id)
{
  header_.network = network_id;
}

inline void Packet::SetTarget(RawAddress const &address)
{
  std::memcpy(header_.target.data(), address.data(), address.size());
}

inline void Packet::SetTarget(Address const &address)
{
  assert(address.size() == ADDRESS_SIZE);
  std::memcpy(header_.target.data(), address.pointer(), address.size());
}

inline void Packet::SetPayload(Payload const &payload)
{
  payload_ = payload;
}

template <typename T>
void Serialize(T &serializer, Packet const &packet)
{
  serializer << *reinterpret_cast<Packet::BinaryHeader const *>(&packet.header_) << packet.payload_;
}

template <typename T>
void Deserialize(T &serializer, Packet &packet)
{
  serializer >> *reinterpret_cast<Packet::BinaryHeader *>(&packet.header_) >> packet.payload_;
}

}  // namespace muddle
}  // namespace fetch

namespace std {

template <>
struct hash<fetch::muddle::Packet::RawAddress>
{
  std::size_t operator()(fetch::muddle::Packet::RawAddress const &address) const noexcept
  {
    uint32_t hash = 2166136261;
    for (std::size_t i = 0; i < address.size(); ++i)
    {
      hash = (hash * 16777619) ^ address[i];
    }
    return hash;
  }
};

}  // namespace std
