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
#include "core/serializers/byte_array_buffer.hpp"
#include "crypto/prover.hpp"
#include "crypto/verifier.hpp"

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
 * │                                                                 │
 *
 * │                                                                 │
 *
 * └ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┘
 * │                          Stamp (if any)                         │
 *
 * └ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┘
 */
class Packet
{
public:
  static constexpr std::size_t ADDRESS_SIZE = 64;

  using RawAddress = std::array<uint8_t, ADDRESS_SIZE>;
  using Address    = byte_array::ConstByteArray;
  using Payload    = byte_array::ConstByteArray;
  using Stamp      = byte_array::ConstByteArray;

  struct RoutingHeader
  {
    // clang-format off
    uint32_t version   : 4;   ///< Flag to signal the current version of the muddle protocol
    uint32_t direct    : 1;   ///< Flag to signal that a direct message is being sent (no routing)
    uint32_t broadcast : 1;   ///< Flag to signal that the packet is a broadcast packet
    uint32_t exchange  : 1;   ///< Flag to signal that this is an exchange packet
    uint32_t stamped   : 1;   ///< Flag to signal that the packet is signed by sender
    uint32_t ttl       : 8;   ///< The time to live counter
    uint32_t service   : 16;  ///< The service number (helpful for RPC compatibility)
    uint32_t proto     : 16;  ///< The protocol number (helpful for RPC compatibility)
    uint32_t msg_num   : 16;  ///< Incremented message counter for detecting duplicate packets
    uint32_t network   : 32;  ///< The originating network id
    // clang-format on

    RawAddress target;  ///< The address of the packet target
    RawAddress sender;  ///< The address of the packet sender
  };

  static constexpr std::size_t HEADER_SIZE = sizeof(RoutingHeader);
  using BinaryHeader                       = std::array<uint8_t, HEADER_SIZE>;

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
  uint8_t           GetVersion() const noexcept;
  bool              IsDirect() const noexcept;
  bool              IsBroadcast() const noexcept;
  bool              IsExchange() const noexcept;
  bool              IsStamped() const noexcept;
  uint8_t           GetTTL() const noexcept;
  uint16_t          GetService() const noexcept;
  uint16_t          GetProtocol() const noexcept;
  uint16_t          GetMessageNum() const noexcept;
  uint32_t          GetNetworkId() const noexcept;
  RawAddress const &GetTargetRaw() const noexcept;
  RawAddress const &GetSenderRaw() const noexcept;
  Address const &   GetTarget() const;
  Address const &   GetSender() const;
  Payload const &   GetPayload() const noexcept;
  Stamp const &     GetStamp() const noexcept;

  // Setters
  void SetDirect(bool set = true) noexcept;
  void SetBroadcast(bool set = true) noexcept;
  void SetExchange(bool set = true) noexcept;
  void SetTTL(uint8_t ttl) noexcept;
  void SetService(uint16_t service_num) noexcept;
  void SetProtocol(uint16_t protocol_num) noexcept;
  void SetMessageNum(uint16_t message_num) noexcept;
  void SetNetworkId(uint32_t network_id) noexcept;
  void SetTarget(RawAddress const &address);
  void SetTarget(Address const &address);
  void SetPayload(Payload const &payload);

  void Sign(crypto::Prover &prover);
  bool Verify() const;

private:
  RoutingHeader header_;   ///< The header containing primarily routing information
  Payload       payload_;  ///< The payload of the message
  Stamp         stamp_;    ///< Signature when stamped

  ///< Cached versions of the addresses
  mutable Address target_;
  mutable Address sender_;

  void         SetStamped(bool set = true) noexcept;
  BinaryHeader StaticHeader() const noexcept;

  template <typename V, typename D>
  friend struct serializers::MapSerializer;
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

inline uint8_t Packet::GetVersion() const noexcept
{
  return static_cast<uint8_t>(header_.version);
}

inline bool Packet::IsDirect() const noexcept
{
  return header_.direct > 0;
}

inline bool Packet::IsBroadcast() const noexcept
{
  return header_.broadcast > 0;
}

inline bool Packet::IsExchange() const noexcept
{
  return header_.exchange > 0;
}

inline bool Packet::IsStamped() const noexcept
{
  return header_.stamped;
}

inline uint8_t Packet::GetTTL() const noexcept
{
  return static_cast<uint8_t>(header_.ttl);
}

inline uint16_t Packet::GetService() const noexcept
{
  return static_cast<uint16_t>(header_.service);
}

inline uint16_t Packet::GetProtocol() const noexcept
{
  return static_cast<uint16_t>(header_.proto);
}

inline uint16_t Packet::GetMessageNum() const noexcept
{
  return static_cast<uint16_t>(header_.msg_num);
}

inline uint32_t Packet::GetNetworkId() const noexcept
{
  return header_.network;
}

inline Packet::RawAddress const &Packet::GetTargetRaw() const noexcept
{
  return header_.target;
}

inline Packet::RawAddress const &Packet::GetSenderRaw() const noexcept
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

inline Packet::Payload const &Packet::GetPayload() const noexcept
{
  return payload_;
}

inline Packet::Stamp const &Packet::GetStamp() const noexcept
{
  return stamp_;
}

inline void Packet::SetDirect(bool set) noexcept
{
  header_.direct = (set) ? 1 : 0;
  SetStamped(false);
}

inline void Packet::SetBroadcast(bool set) noexcept
{
  header_.broadcast = (set) ? 1 : 0;
  SetStamped(false);
}

inline void Packet::SetExchange(bool set) noexcept
{
  header_.exchange = (set) ? 1 : 0;
  SetStamped(false);
}

inline void Packet::SetTTL(uint8_t ttl) noexcept
{
  header_.ttl = ttl;
  // stamps are not invalidated
}

inline void Packet::SetService(uint16_t service_num) noexcept
{
  header_.service = service_num;
  SetStamped(false);
}

inline void Packet::SetProtocol(uint16_t protocol_num) noexcept
{
  header_.proto = protocol_num;
  SetStamped(false);
}

inline void Packet::SetMessageNum(uint16_t message_num) noexcept
{
  header_.msg_num = message_num;
  SetStamped(false);
}

inline void Packet::SetNetworkId(uint32_t network_id) noexcept
{
  header_.network = network_id;
  SetStamped(false);
}

inline void Packet::SetTarget(RawAddress const &address)
{
  std::memcpy(header_.target.data(), address.data(), address.size());
  SetStamped(false);
}

inline void Packet::SetTarget(Address const &address)
{
  assert(address.size() == ADDRESS_SIZE);
  std::memcpy(header_.target.data(), address.pointer(), address.size());
  SetStamped(false);
}

inline void Packet::SetPayload(Payload const &payload)
{
  payload_ = payload;
  SetStamped(false);
}

inline void Packet::SetStamped(bool set) noexcept
{
  header_.stamped = set;
}

inline Packet::BinaryHeader Packet::StaticHeader() const noexcept
{
  RoutingHeader retVal{header_};
  retVal.ttl = 0;
  return *reinterpret_cast<BinaryHeader const *>(&retVal);
}

inline void Packet::Sign(crypto::Prover &prover)
{
  SetStamped();

  auto const signature =
      prover.Sign((serializers::ByteArrayBuffer() << StaticHeader() << payload_).data());

  if (!signature.empty())
  {
    stamp_ = signature;
  }
  else
  {
    SetStamped(false);
  }
}

inline bool Packet::Verify() const
{
  if (!IsStamped())
  {
    return false;  // null signature is not genuine in non-trusted networks
  }
  auto retVal = crypto::Verify(
      GetSender(), (serializers::ByteArrayBuffer() << StaticHeader() << payload_).data(), stamp_);
  return retVal;
}

}  // namespace muddle

namespace serializers {
template <typename D>
struct MapSerializer<muddle::Packet, D>
{
public:
  using Type       = muddle::Packet;
  using DriverType = D;

  static const uint8_t HEADER  = 1;
  static const uint8_t PAYLOAD = 2;
  static const uint8_t STAMP   = 3;

  template <typename T>
  static void Serialize(T &map_constructor, Type const &packet)
  {
    auto map = map_constructor(3);
    map.Append(HEADER, *reinterpret_cast<Type::BinaryHeader const *>(&packet.header_));
    map.Append(PAYLOAD, packet.payload_);
    map.Append(STAMP,   packet.stamp_);  // TODO: add support optional fields.
    /*
    serializer << *reinterpret_cast<Packet::BinaryHeader const *>(&packet.header_) <<
    packet.payload_; if (packet.header_.stamped)
    {
      serializer << packet.stamp_;
    }
    */
  }

  template <typename T>
  static void Deserialize(T &map, Type &packet)
  {
    if(map.size() != 3)
    {
      throw std::runtime_error("Packet must have exactly 3 elements, but " + std::to_string(map.size()) + " found.");
    }
    map.ExpectKeyGetValue(HEADER, *reinterpret_cast<Type::BinaryHeader *>(&packet.header_));
    map.ExpectKeyGetValue(PAYLOAD, packet.payload_);
    map.ExpectKeyGetValue(STAMP, packet.stamp_);  // TODO: add support optional fields.
    /*
    serializer >> *reinterpret_cast<Packet::BinaryHeader *>(&packet.header_) >> packet.payload_;
    if (packet.header_.stamped)
    {
      serializer >> packet.stamp_;
    }
    */
  }
};
}  // namespace serializers

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
