#pragma once
#include "core/byte_array/byte_array.hpp"
#include "core/serializers/base_types.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/main_serializer.hpp"
#include "muddle/address.hpp"

namespace fetch {
namespace agent {

struct NetworkLocation
{
  using Address = muddle::Address;

  Address node;
  Address agent;

  bool operator==(NetworkLocation const &other) const
  {
    return (node == other.node) && (agent == other.agent);
  }
};

struct Message
{
  using ConstByteArray = byte_array::ConstByteArray;

  NetworkLocation from;
  NetworkLocation to;

  ConstByteArray protocol;
  ConstByteArray payload;
  bool           operator==(Message const &other) const
  {
    return (from == other.from) && (to == other.to) && (protocol == other.protocol) &&
           (payload == other.payload);
  }
};

}  // namespace agent

namespace serializers {

template <typename D>
struct MapSerializer<agent::NetworkLocation, D>
{
public:
  using Type       = agent::NetworkLocation;
  using DriverType = D;

  static uint8_t const NODE  = 1;
  static uint8_t const AGENT = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &location)
  {
    auto map = map_constructor(2);
    map.Append(NODE, location.node);
    map.Append(AGENT, location.agent);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &location)
  {
    map.ExpectKeyGetValue(NODE, location.node);
    map.ExpectKeyGetValue(AGENT, location.agent);
  }
};

template <typename D>
struct MapSerializer<agent::Message, D>
{
public:
  using Type       = agent::Message;
  using DriverType = D;

  static uint8_t const FROM     = 1;
  static uint8_t const TO       = 2;
  static uint8_t const PROTOCOL = 3;
  static uint8_t const PAYLOAD  = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &message)
  {
    auto map = map_constructor(4);
    map.Append(FROM, message.from);
    map.Append(TO, message.to);
    map.Append(PROTOCOL, message.protocol);
    map.Append(PAYLOAD, message.payload);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &message)
  {
    map.ExpectKeyGetValue(FROM, message.from);
    map.ExpectKeyGetValue(TO, message.to);
    map.ExpectKeyGetValue(PROTOCOL, message.protocol);
    map.ExpectKeyGetValue(PAYLOAD, message.payload);
  }
};

}  // namespace serializers

}  // namespace fetch
