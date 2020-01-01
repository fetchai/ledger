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
#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "muddle/rbc_messages.hpp"

#include <cstdint>

namespace fetch {
namespace muddle {

/// @name Getter functions
/// @{
/**
 * Creates unique tag for the message out of channel_, id_ and
 * message counter
 *
 * @return Tag of message
 */
TagType RBCMessage::tag() const
{
  TagType msg_tag = channel_;
  msg_tag <<= 48;
  msg_tag |= id_;
  msg_tag <<= 32;
  return (msg_tag | uint64_t(counter_));
}

uint16_t RBCMessage::channel() const
{
  return static_cast<uint16_t>(channel_);
}

uint8_t RBCMessage::counter() const
{
  return counter_;
}

uint32_t RBCMessage::id() const
{
  return id_;
}
/// @}

RBCSerializer RBCMessage::Serialize() const
{
  RBCSerializer serialiser;
  serialiser << *this;
  return serialiser;
}

RBCMessageType RBCMessage::type() const
{
  return type_;
}

SerialisedMessage const &RBCMessage::message() const
{
  return payload_;
}

HashDigest RBCMessage::hash() const
{
  return payload_;
}

bool RBCMessage::is_valid() const
{
  // Any RBC message created without a explict C++ type is not valid.
  return false;
}

RBCMessage::RBCMessage(RBCMessageType type, uint16_t channel, uint32_t id, uint8_t counter,
                       SerialisedMessage msg)
  : type_{type}
  , channel_{channel}
  , id_{id}
  , counter_{counter}
  , payload_(std::move(msg))
{}

}  // namespace muddle
}  // namespace fetch
