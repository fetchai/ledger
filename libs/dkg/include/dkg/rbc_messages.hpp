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
#include "crypto/bls_base.hpp"

#include <cstdint>

namespace fetch {
namespace dkg {
namespace rbc {

using TruncatedHash        = byte_array::ByteArray;
using TagType              = uint64_t;
using SerialisedMessage    = byte_array::ConstByteArray;
using RBCSerializer        = fetch::serializers::ByteArrayBuffer;
using RBCSerializerCounter = fetch::serializers::SizeCounter<fetch::serializers::ByteArrayBuffer>;

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

  TagType Tag() const
  {
    TagType msg_tag = channel_id_;
    msg_tag <<= 48;
    msg_tag |= node_id_;
    msg_tag <<= 32;
    return (msg_tag | uint64_t(sequence_counter_));
  }
  RBCMessage::MessageType Type() const
  {
    return type_;
  }
  uint8_t ChannelId() const
  {
    return static_cast<uint8_t>(channel_id_);
  }
  uint8_t SeqCounter() const
  {
    return sequence_counter_;
  }
  uint32_t Id() const
  {
    return node_id_;
  }
  virtual RBCSerializer Serialize() const = 0;

protected:
  const MessageType type_;
  // Tag of message
  uint16_t channel_id_{0};  // Is this safe to truncate to uint8_t?
  uint32_t node_id_{0};     // Should this be crypto::bls::Id?
  uint8_t  sequence_counter_{0};

  explicit RBCMessage(MessageType type)
    : type_{type} {};
  explicit RBCMessage(uint16_t channel, uint32_t node, uint8_t seq, MessageType type)
    : type_{type}
    , channel_id_{channel}
    , node_id_{node}
    , sequence_counter_{seq} {};
};

class RMessage : public RBCMessage
{
public:
  RBCSerializer Serialize() const override
  {
    RBCSerializer serialiser;
    serialiser << channel_id_ << node_id_ << sequence_counter_ << message_;
    return serialiser;
  }

  // For tests:
  const SerialisedMessage &Message() const
  {
    return message_;
  }

protected:
  SerialisedMessage message_;
  explicit RMessage(MessageType type)
    : RBCMessage{type} {};
  RMessage(uint16_t channel, uint32_t node, uint8_t seq, SerialisedMessage msg, MessageType type)
    : RBCMessage{channel, node, seq, type}
    , message_{std::move(msg)} {};
};

class RHash : public RBCMessage
{
public:
  RBCSerializer Serialize() const override
  {
    RBCSerializer serialiser;
    serialiser << channel_id_ << node_id_ << sequence_counter_ << hash_;
    return serialiser;
  }

  // For tests:
  const TruncatedHash &Hash() const
  {
    return hash_;
  }

protected:
  TruncatedHash hash_;
  explicit RHash(MessageType type)
    : RBCMessage{type} {};
  RHash(uint16_t channel, uint32_t node, uint8_t seq, TruncatedHash msg_hash, MessageType type)
    : RBCMessage{channel, node, seq, type}
    , hash_{std::move(msg_hash)} {};
};

// Different RBC Messages:

// Create this message for messages that need to be broadcasted
class RBroadcast : public RMessage
{
public:
  explicit RBroadcast(uint16_t channel, uint32_t node, uint8_t seq_counter, SerialisedMessage msg)
    : RMessage{channel, node, seq_counter, std::move(msg), MessageType::RBROADCAST} {};
  explicit RBroadcast(RBCSerializer &serialiser)
    : RMessage{MessageType::RBROADCAST}
  {
    serialiser >> channel_id_ >> node_id_ >> sequence_counter_ >> message_;
  }
};

// Send an REcho when we receive an RBroadcast message
class REcho : public RHash
{
public:
  explicit REcho(uint16_t channel, uint32_t node, uint8_t seq_counter, TruncatedHash msg_hash)
    : RHash{channel, node, seq_counter, std::move(msg_hash), MessageType::RECHO} {};
  explicit REcho(RBCSerializer &serialiser)
    : RHash{MessageType::RECHO}
  {
    serialiser >> channel_id_ >> node_id_ >> sequence_counter_ >> hash_;
  }
};

// RReady messages which are sent once we receive num_parties - threshold REcho's
class RReady : public RHash
{
public:
  explicit RReady(uint16_t channel, uint32_t node, uint8_t seq_counter, TruncatedHash msg_hash)
    : RHash{channel, node, seq_counter, std::move(msg_hash), MessageType::RREADY} {};
  explicit RReady(RBCSerializer &serialiser)
    : RHash{MessageType::RREADY}
  {
    serialiser >> channel_id_ >> node_id_ >> sequence_counter_ >> hash_;
  }
};

// Make RRequest if the hash of RReady messages does not match our RBroadcast message
class RRequest : public RBCMessage
{
public:
  explicit RRequest(uint16_t channel, uint32_t node, uint8_t seq_counter)
    : RBCMessage{channel, node, seq_counter, MessageType::RREQUEST} {};
  explicit RRequest(RBCSerializer &serialiser)
    : RBCMessage{MessageType::RREQUEST}
  {
    serialiser >> channel_id_ >> node_id_ >> sequence_counter_;
  }

  RBCSerializer Serialize() const override
  {
    RBCSerializer serialiser;
    serialiser << channel_id_ << node_id_ << sequence_counter_;
    return serialiser;
  }
};

// Reply to RRequest messages
class RAnswer : public RMessage
{
public:
  explicit RAnswer(uint16_t channel, uint32_t node, uint8_t seq_counter, SerialisedMessage msg)
    : RMessage{channel, node, seq_counter, std::move(msg), MessageType::RANSWER} {};
  explicit RAnswer(RBCSerializer &serialiser)
    : RMessage{MessageType::RANSWER}
  {
    serialiser >> channel_id_ >> node_id_ >> sequence_counter_ >> message_;
  }
};
}  // namespace rbc
}  // namespace dkg
}  // namespace fetch
