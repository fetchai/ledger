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
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"

#include <cstdint>

namespace fetch {
namespace dkg {
namespace rbc {

using TruncatedHash        = byte_array::ByteArray;
using TagType              = uint64_t;
using SerialisedMessage    = byte_array::ConstByteArray;
using RBCSerializer        = fetch::serializers::ByteArrayBuffer;
using RBCSerializerCounter = fetch::serializers::SizeCounter<fetch::serializers::ByteArrayBuffer>;

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
    RBROADCAST,
    RECHO,
    RREADY,
    RREQUEST,
    RANSWER
  };

  // Destruction
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

  RBCMessage::MessageType type() const
  {
    return type_;
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

  virtual RBCSerializer Serialize() const = 0;

protected:
  const MessageType type_;  ///< Type of the message
  uint16_t
           channel_;  ///< Channel Id of the broadcast channel (is this safe to truncate to uint8_t?)
  uint32_t id_;       ///< Unique Id of the node
  uint8_t  counter_;  ///< Counter for messages sent on RBC

  explicit RBCMessage(MessageType type)
    : type_{type}
  {}
  RBCMessage(uint16_t channel, uint32_t id, uint8_t counter, MessageType type)
    : type_{type}
    , channel_{channel}
    , id_{id}
    , counter_{counter}
  {}
};

class RMessage : public RBCMessage
{
public:
  // Destruction
  virtual ~RMessage() = default;

  RBCSerializer Serialize() const override
  {
    RBCSerializer serialiser;
    serialiser << channel_ << id_ << counter_ << message_;
    return serialiser;
  }

  const SerialisedMessage &message() const
  {
    return message_;
  }

protected:
  SerialisedMessage message_;  ///< Serialised message to be sent using RBC

  explicit RMessage(MessageType type)
    : RBCMessage{type}
  {}
  RMessage(uint16_t channel, uint32_t id, uint8_t counter, SerialisedMessage msg, MessageType type)
    : RBCMessage{channel, id, counter, type}
    , message_{std::move(msg)}
  {}
};

class RHash : public RBCMessage
{
public:
  // Destruction
  virtual ~RHash() = default;

  RBCSerializer Serialize() const override
  {
    RBCSerializer serialiser;
    serialiser << channel_ << id_ << counter_ << hash_;
    return serialiser;
  }

  const TruncatedHash &hash() const
  {
    return hash_;
  }

protected:
  TruncatedHash hash_;  ///< Truncated hash of serialised message

  // Construction
  explicit RHash(MessageType type)
    : RBCMessage{type}
  {}
  RHash(uint16_t channel, uint32_t id, uint8_t counter, TruncatedHash msg_hash, MessageType type)
    : RBCMessage{channel, id, counter, type}
    , hash_{std::move(msg_hash)}
  {}
};

class RBroadcast : public RMessage
{
public:
  RBroadcast(uint16_t channel, uint32_t id, uint8_t counter, SerialisedMessage msg)
    : RMessage{channel, id, counter, std::move(msg), MessageType::RBROADCAST}
  {}
  explicit RBroadcast(RBCSerializer &serialiser)
    : RMessage{MessageType::RBROADCAST}
  {
    serialiser >> channel_ >> id_ >> counter_ >> message_;
  }
  ~RBroadcast() override = default;
};

class REcho : public RHash
{
public:
  REcho(uint16_t channel, uint32_t id, uint8_t counter, TruncatedHash msg_hash)
    : RHash{channel, id, counter, std::move(msg_hash), MessageType::RECHO}
  {}
  explicit REcho(RBCSerializer &serialiser)
    : RHash{MessageType::RECHO}
  {
    serialiser >> channel_ >> id_ >> counter_ >> hash_;
  }
  ~REcho() override = default;
};

class RReady : public RHash
{
public:
  // Construction/Destruction
  RReady(uint16_t channel, uint32_t id, uint8_t counter, TruncatedHash msg_hash)
    : RHash{channel, id, counter, std::move(msg_hash), MessageType::RREADY}
  {}
  explicit RReady(RBCSerializer &serialiser)
    : RHash{MessageType::RREADY}
  {
    serialiser >> channel_ >> id_ >> counter_ >> hash_;
  }
  ~RReady() override = default;
};

class RRequest : public RBCMessage
{
public:
  RRequest(uint16_t channel, uint32_t id, uint8_t counter)
    : RBCMessage{channel, id, counter, MessageType::RREQUEST}
  {}
  explicit RRequest(RBCSerializer &serialiser)
    : RBCMessage{MessageType::RREQUEST}
  {
    serialiser >> channel_ >> id_ >> counter_;
  }
  ~RRequest() override = default;

  RBCSerializer Serialize() const override
  {
    RBCSerializer serialiser;
    serialiser << channel_ << id_ << counter_;
    return serialiser;
  }
};

class RAnswer : public RMessage
{
public:
  RAnswer(uint16_t channel, uint32_t id, uint8_t counter, SerialisedMessage msg)
    : RMessage{channel, id, counter, std::move(msg), MessageType::RANSWER}
  {}
  explicit RAnswer(RBCSerializer &serialiser)
    : RMessage{MessageType::RANSWER}
  {
    serialiser >> channel_ >> id_ >> counter_ >> message_;
  }
  ~RAnswer() override = default;
};
}  // namespace rbc
}  // namespace dkg
}  // namespace fetch
