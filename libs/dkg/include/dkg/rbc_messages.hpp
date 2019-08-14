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

#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"

#include <cstdint>

namespace fetch {
namespace dkg {

using TruncatedHash        = byte_array::ByteArray;
using TagType              = uint64_t;
using SerialisedMessage    = byte_array::ConstByteArray;
using RBCSerializer        = fetch::serializers::MsgPackSerializer;
using RBCSerializerCounter = fetch::serializers::SizeCounter;

/**
 * Different messages using in reliable broadcast channel(RBC).
 * RBroadcast - container for message to be sent using RBC and triggers the protocol
 * REcho - reply to RBroadcast message, containing hash of message
 * RReady - message signalling the receipt of protocol specified number of REcho's
 * RRequest - request for original message if the hash of RReady messages does not match our
 * RBroadcast message RAnswer - reply to RRequest message
 */

class RBCMessage
{
public:
  enum class MessageType : uint8_t
  {
    R_BROADCAST,
    R_ECHO,
    R_READY,
    R_REQUEST,
    R_ANSWER,

    R_INVALID
  };

  RBCMessage() = default;
  /// @name Getter functions
  /// @{
  /**
   * Creates unique tag for the message out of channel_, id_ and
   * message counter
   *
   * @return Tag of message
   */
  TagType tag() const
  {
    TagType msg_tag = channel_;
    msg_tag <<= 48;
    msg_tag |= id_;
    msg_tag <<= 32;
    return (msg_tag | uint64_t(counter_));
  }

  uint8_t channel() const
  {
    return static_cast<uint8_t>(channel_);
  }

  uint8_t counter() const
  {
    return counter_;
  }

  uint32_t id() const
  {
    return id_;
  }
  /// @}

  RBCSerializer Serialize() const;

  MessageType type() const
  {
    return type_;
  }

  SerialisedMessage const &message() const
  {
    return payload_;
  }

  TruncatedHash hash() const
  {
    return payload_;
  }

  bool is_valid() const
  {
    return type_ != MessageType::R_INVALID;
  }

  template <typename T, typename D>
  friend struct serializers::MapSerializer;

protected:
  // Destruction
  RBCMessage(MessageType type, uint16_t channel = 0, uint32_t id = 0, uint8_t counter = 0,
             SerialisedMessage msg = "")
    : type_{type}
    , channel_{channel}
    , id_{id}
    , counter_{counter}
    , payload_(std::move(msg))
  {}

private:
  MessageType type_{MessageType::R_INVALID};
  uint16_t
                    channel_;  ///< Channel Id of the broadcast channel (is this safe to truncate to uint8_t?)
  uint32_t          id_;       ///< Unique Id of the node
  uint8_t           counter_;  ///< Counter for messages sent on RBC
  SerialisedMessage payload_;  ///< Serialised message to be sent using RBC
};

template <RBCMessage::MessageType TYPE>
class RBCMessageImpl final : public RBCMessage
{
public:
  RBCMessageImpl(uint16_t channel = 0, uint32_t id = 0, uint8_t counter = 0,
                 SerialisedMessage msg = "")
    : RBCMessage{TYPE, channel, id, counter, msg}
  {}
};

using RMessage   = RBCMessage;
using RHash      = RBCMessage;
using RBroadcast = RBCMessageImpl<RBCMessage::MessageType::R_BROADCAST>;
using RRequest   = RBCMessageImpl<RBCMessage::MessageType::R_REQUEST>;
using RAnswer    = RBCMessageImpl<RBCMessage::MessageType::R_ANSWER>;
using REcho      = RBCMessageImpl<RBCMessage::MessageType::R_ECHO>;
using RReady     = RBCMessageImpl<RBCMessage::MessageType::R_READY>;

}  // namespace dkg

namespace serializers {
template <typename D>
struct MapSerializer<dkg::RBCMessage, D>
{
public:
  using Type       = dkg::RBCMessage;
  using DriverType = D;

  static uint8_t const TYPE    = 1;
  static uint8_t const CHANNEL = 2;
  static uint8_t const ADDRESS = 3;
  static uint8_t const COUNTER = 4;
  static uint8_t const PAYLOAD = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &msg)
  {
    auto map = map_constructor(5);
    map.Append(TYPE, static_cast<uint8_t>(msg.type_));
    map.Append(CHANNEL, msg.channel_);
    map.Append(ADDRESS, msg.id_);  // TODO: Remove and deduce from network connection
    map.Append(COUNTER, msg.counter_);
    map.Append(PAYLOAD, msg.payload_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &msg)
  {
    uint8_t type;
    map.ExpectKeyGetValue(TYPE, type);
    map.ExpectKeyGetValue(CHANNEL, msg.channel_);
    map.ExpectKeyGetValue(ADDRESS, msg.id_);  // TODO: Remove and deduce from network connection
    map.ExpectKeyGetValue(COUNTER, msg.counter_);
    map.ExpectKeyGetValue(PAYLOAD, msg.payload_);

    msg.type_ = static_cast<Type::MessageType>(type);
  }
};
}  // namespace serializers

namespace dkg {

inline RBCSerializer RBCMessage::Serialize() const
{
  RBCSerializer serialiser;
  serialiser << *this;
  return serialiser;
}

}  // namespace dkg
}  // namespace fetch
