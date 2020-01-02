#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "core/mutex.hpp"
#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"

#include <cstdint>
#include <memory>

namespace fetch {
namespace muddle {

using HashDigest           = byte_array::ByteArray;
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
  R_BROADCAST = 1,
  R_ECHO,
  R_READY,
  R_REQUEST,
  R_ANSWER
};

template <RBCMessageType TYPE, typename Parent>
class RBCMessageImpl;
class RHash;
class RMessage;

using RBroadcast = RBCMessageImpl<RBCMessageType::R_BROADCAST, RMessage>;
using RRequest   = RBCMessageImpl<RBCMessageType::R_REQUEST, RMessage>;
using RAnswer    = RBCMessageImpl<RBCMessageType::R_ANSWER, RMessage>;
using REcho      = RBCMessageImpl<RBCMessageType::R_ECHO, RHash>;
using RReady     = RBCMessageImpl<RBCMessageType::R_READY, RHash>;

using MessageContents = std::shared_ptr<RMessage>;
using MessageHash     = std::shared_ptr<RHash>;

using MessageBroadcast = std::shared_ptr<RBroadcast>;
using MessageRequest   = std::shared_ptr<RRequest>;
using MessageAnswer    = std::shared_ptr<RAnswer>;
using MessageEcho      = std::shared_ptr<REcho>;
using MessageReady     = std::shared_ptr<RReady>;

class RBCMessage
{
public:
  using SharedRBCMessage = std::shared_ptr<RBCMessage>;

  template <typename... Args>
  static SharedRBCMessage New(RBCMessageType type, Args... args)
  {
    SharedRBCMessage ret{nullptr};

    NewType([&ret](auto value) { ret = value; }, type, std::forward<Args>(args)...);

    return ret;
  }

  template <typename T, typename... Args>
  static std::shared_ptr<T> New(Args... args)
  {
    std::shared_ptr<T> ret{nullptr};
    ret.reset(new T(std::forward<Args>(args)...));
    return ret;
  }

  template <typename F, typename... Args>
  static bool NewType(F f, RBCMessageType type, Args... args)
  {
    switch (type)
    {
    case RBCMessageType::R_BROADCAST:
      f(New<RBroadcast>(std::forward<Args>(args)...));
      break;
    case RBCMessageType::R_ECHO:
      f(New<REcho>(std::forward<Args>(args)...));
      break;
    case RBCMessageType::R_READY:
      f(New<RReady>(std::forward<Args>(args)...));
      break;
    case RBCMessageType::R_REQUEST:
      f(New<RRequest>(std::forward<Args>(args)...));
      break;
    case RBCMessageType::R_ANSWER:
      f(New<RAnswer>(std::forward<Args>(args)...));
      break;
    default:
      return false;
    }

    return true;
  }

  // Constructs an invalid message
  RBCMessage() = default;

  /// Properties
  /// @{
  TagType        tag() const;
  uint16_t       channel() const;
  uint8_t        counter() const;
  uint32_t       id() const;
  RBCMessageType type() const;
  /// @}

  /// Validation
  /// @{
  virtual bool is_valid() const;
  /// @}

  RBCSerializer Serialize() const;

  template <typename T, typename D>
  friend struct serializers::MapSerializer;

  RBCMessage(RBCMessage const &) = default;  // TODO (troels): make protected

  /// @{
  SerialisedMessage const &message() const;
  HashDigest               hash() const;
  /// @}
protected:
  explicit RBCMessage(RBCMessageType type, uint16_t channel = 0, uint32_t id = 0,
                      uint8_t counter = 0, SerialisedMessage msg = "");

private:
  RBCMessageType    type_;
  uint16_t          channel_{};  ///< Channel Id of the broadcast channel
  uint32_t          id_{};       ///< Unique Id of the node
  uint8_t           counter_{};  ///< Counter for messages sent on RBC
  SerialisedMessage payload_;    ///< Serialised message to be sent using RBC
};

class RHash : public RBCMessage
{
public:
  explicit RHash(RBCMessage const &msg)
    : RBCMessage(msg)
  {}
  using RBCMessage::hash;
  using RBCMessage::RBCMessage;
};

class RMessage : public RBCMessage
{
public:
  explicit RMessage(RBCMessage const &msg)
    : RBCMessage(msg)
  {}
  using RBCMessage::message;
  using RBCMessage::RBCMessage;
};

template <RBCMessageType TYPE, typename Parent>
class RBCMessageImpl final : public Parent
{
public:
  explicit RBCMessageImpl(uint16_t channel = 0, uint32_t id = 0, uint8_t counter = 0,
                          SerialisedMessage msg = "")
    : Parent{TYPE, channel, id, counter, msg}
  {}

  explicit RBCMessageImpl(RBCMessage const &msg)
    : Parent(msg)
  {}

  bool is_valid() const override
  {
    return this->type() == TYPE;
  }
};

}  // namespace muddle

namespace serializers {
template <typename D>
struct MapSerializer<muddle::RBCMessage, D>
{
public:
  using Type       = muddle::RBCMessage;
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
    map.Append(ADDRESS, msg.id_);  // TODO (ed): Remove and deduce from network connection
    map.Append(COUNTER, msg.counter_);
    map.Append(PAYLOAD, msg.payload_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &msg)
  {
    uint8_t type;
    map.ExpectKeyGetValue(TYPE, type);
    map.ExpectKeyGetValue(CHANNEL, msg.channel_);
    map.ExpectKeyGetValue(ADDRESS,
                          msg.id_);  // TODO (ed): Remove and deduce from network connection
    map.ExpectKeyGetValue(COUNTER, msg.counter_);
    map.ExpectKeyGetValue(PAYLOAD, msg.payload_);

    msg.type_ = static_cast<muddle::RBCMessageType>(type);
  }
};
}  // namespace serializers

}  // namespace fetch
