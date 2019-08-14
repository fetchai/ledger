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
    R_SEND
  };

  // Destruction
  RBCMessage() = default;
  RBCMessage(uint16_t channel, uint32_t id, uint8_t counter, SerialisedMessage msg = "")
    : channel_{channel}
    , id_{id}
    , counter_{counter}
    , payload_(std::move(msg))
  {}
  explicit RBCMessage(RBCSerializer &serialiser)
  {
    serialiser >> channel_ >> id_ >> counter_ >> payload_;
  }

  virtual ~RBCMessage() = default;

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

  RBCSerializer Serialize() const
  {
    RBCSerializer serialiser;
    serialiser << channel_ << id_ << counter_ << payload_;
    return serialiser;
  }

  virtual MessageType type() const = 0;

protected:
  SerialisedMessage const &message() const
  {
    return payload_;
  }

  TruncatedHash hash() const
  {
    return payload_;
  }

private:
  uint16_t
                    channel_;  ///< Channel Id of the broadcast channel (is this safe to truncate to uint8_t?)
  uint32_t          id_;       ///< Unique Id of the node
  uint8_t           counter_;  ///< Counter for messages sent on RBC
  SerialisedMessage payload_;  ///< Serialised message to be sent using RBC
};

class RMessage : public RBCMessage
{
public:
  using RBCMessage::RBCMessage;
  virtual ~RMessage() = default;
  using RBCMessage::message;
};

template <RBCMessage::MessageType TYPE>
class RMessageImpl final : public RMessage
{
public:
  using RMessage::RMessage;
  virtual ~RMessageImpl() = default;

  MessageType type() const override
  {
    return TYPE;
  }
};

class RHash : public RBCMessage
{
public:
  // Destruction
  using RBCMessage::hash;
  using RBCMessage::RBCMessage;
  virtual ~RHash() = default;
};

template <RBCMessage::MessageType TYPE>
class RHashImpl final : public RHash
{
public:
  using RHash::RHash;
  virtual ~RHashImpl() = default;

  MessageType type() const override
  {
    return TYPE;
  }
};

using RBroadcast = RMessageImpl<RBCMessage::MessageType::R_BROADCAST>;
using RRequest   = RMessageImpl<RBCMessage::MessageType::R_REQUEST>;
using RAnswer    = RMessageImpl<RBCMessage::MessageType::R_ANSWER>;
using REcho      = RHashImpl<RBCMessage::MessageType::R_ECHO>;
using RReady     = RHashImpl<RBCMessage::MessageType::R_READY>;

}  // namespace dkg
}  // namespace fetch
