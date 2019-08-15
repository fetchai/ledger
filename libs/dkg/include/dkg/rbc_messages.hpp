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
#include <memory>

namespace fetch {
namespace dkg {

using MessageHash          = byte_array::ByteArray;
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

enum class RBCMessageType : uint8_t
{
  R_BROADCAST,
  R_ECHO,
  R_READY,
  R_REQUEST,
  R_ANSWER
};

template <RBCMessageType TYPE>
class RBCMessageImpl;

class RBCMessage
{
public:
  using SharedRBCMessage = std::shared_ptr<RBCMessage>;

  template <typename... Args>
  static SharedRBCMessage New(RBCMessageType type, Args... args)
  {
    SharedRBCMessage ret{nullptr};

    switch (type)
    {
    case RBCMessageType::R_BROADCAST:
      ret = New<RBCMessageType::R_BROADCAST>(std::forward<Args>(args)...);
      break;
    case RBCMessageType::R_ECHO:
      ret = New<RBCMessageType::R_ECHO>(std::forward<Args>(args)...);
      break;
    case RBCMessageType::R_READY:
      ret = New<RBCMessageType::R_READY>(std::forward<Args>(args)...);
      break;
    case RBCMessageType::R_REQUEST:
      ret = New<RBCMessageType::R_REQUEST>(std::forward<Args>(args)...);
      break;
    case RBCMessageType::R_ANSWER:
      ret = New<RBCMessageType::R_ANSWER>(std::forward<Args>(args)...);
      break;
    }

    return ret;
  }

  template <RBCMessageType TYPE, typename... Args>
  static SharedRBCMessage New(Args... args)
  {
    SharedRBCMessage ret{nullptr};
    ret.reset(new RBCMessageImpl<TYPE>(std::forward<Args>(args)...));
    return ret;
  }

  static SharedRBCMessage ToNativeType(RBCMessage const &msg)
  {
    return New(msg.type(), msg);
  }

  // Constructs an invalid message
  RBCMessage() = default;

  /// Properties
  /// @{
  TagType                  tag() const;
  uint16_t                 channel() const;
  uint8_t                  counter() const;
  uint32_t                 id() const;
  RBCMessageType           type() const;
  SerialisedMessage const &message() const;
  MessageHash              hash() const;
  /// @}

  /// Validation
  /// @{
  virtual bool is_valid() const;
  /// @}

  RBCSerializer Serialize() const;

  template <typename T, typename D>
  friend struct serializers::MapSerializer;

  RBCMessage(RBCMessage const &) = default;  // TODO: make protected
protected:
  RBCMessage(RBCMessageType type, uint16_t channel = 0, uint32_t id = 0, uint8_t counter = 0,
             SerialisedMessage msg = "");

  RBCMessageType type_;

private:
  uint16_t          channel_;  ///< Channel Id of the broadcast channel
  uint32_t          id_;       ///< Unique Id of the node
  uint8_t           counter_;  ///< Counter for messages sent on RBC
  SerialisedMessage payload_;  ///< Serialised message to be sent using RBC
};

template <RBCMessageType TYPE>
class RBCMessageImpl final : public RBCMessage
{
public:
  RBCMessageImpl(uint16_t channel = 0, uint32_t id = 0, uint8_t counter = 0,
                 SerialisedMessage msg = "")
    : RBCMessage{TYPE, channel, id, counter, msg}
  {}

  RBCMessageImpl(RBCMessage const &msg)
    : RBCMessage(msg)
  {}

  bool is_valid() const override
  {
    return type_ != TYPE;
  }
};

using RMessage   = RBCMessage;
using RHash      = RBCMessage;
using RBroadcast = RBCMessageImpl<RBCMessageType::R_BROADCAST>;
using RRequest   = RBCMessageImpl<RBCMessageType::R_REQUEST>;
using RAnswer    = RBCMessageImpl<RBCMessageType::R_ANSWER>;
using REcho      = RBCMessageImpl<RBCMessageType::R_ECHO>;
using RReady     = RBCMessageImpl<RBCMessageType::R_READY>;

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

    msg.type_ = static_cast<dkg::RBCMessageType>(type);
  }
};
}  // namespace serializers

}  // namespace fetch
