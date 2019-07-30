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

#include "dkg/rbc_messages.hpp"
#include "network/muddle/packet.hpp"

namespace fetch {
namespace dkg {
namespace rbc {

class RBCEnvelope
{
  using MessageType = RBCMessage::MessageType;
  using Payload     = byte_array::ConstByteArray;

public:
  // Constructor
  RBCEnvelope() = default;
  explicit RBCEnvelope(RBCMessage const &msg)
    : type_{msg.type()}
    , payload_{msg.Serialize().data()}
  {}

  std::shared_ptr<RBCMessage> Message() const;

  template <typename T, typename D>
  friend struct serializers::MapSerializer;

private:
  RBCMessage::MessageType type_;     ///< Type of message contained in envelope
  Payload                 payload_;  ///< Serialised RBCMessage
};

}  // namespace rbc
}  // namespace dkg

namespace serializers {
template <typename D>
struct MapSerializer<dkg::rbc::RBCEnvelope, D>
{
public:
  using Type       = dkg::rbc::RBCEnvelope;
  using DriverType = D;

  static uint8_t const TYPE    = 1;
  static uint8_t const MESSAGE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &env)
  {
    auto map = map_constructor(2);
    map.Append(TYPE, static_cast<uint8_t>(env.type_));
    map.Append(MESSAGE, env.payload_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &env)
  {
    uint8_t type;
    map.ExpectKeyGetValue(TYPE, type);
    map.ExpectKeyGetValue(MESSAGE, env.payload_);
    env.type_ = static_cast<dkg::rbc::RBCEnvelope::MessageType>(type);
  }
};

}  // namespace serializers

}  // namespace fetch
