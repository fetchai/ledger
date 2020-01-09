#pragma once
#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/group_definitions.hpp"
#include "semanticsearch/query/error_tracker.hpp"

#include <vector>

namespace fetch {
namespace messenger {

struct QueryResult
{
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using ResultList     = std::vector<ConstByteArray>;
  using ErrorTracker   = semanticsearch::ErrorTracker;

  ConstByteArray message;
  ResultList     agents;
  ErrorTracker   error_tracker;
};

}  // namespace messenger

namespace serializers {

template <typename D>
struct MapSerializer<messenger::QueryResult, D>
{
public:
  using Type       = messenger::QueryResult;
  using DriverType = D;

  static constexpr uint8_t MESSAGE       = 1;
  static constexpr uint8_t AGENTS        = 2;
  static constexpr uint8_t ERROR_TRACKER = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &input)
  {
    auto map = map_constructor(3);
    map.Append(MESSAGE, input.message);
    map.Append(AGENTS, input.agents);
    map.Append(ERROR_TRACKER, input.error_tracker);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &output)
  {
    map.ExpectKeyGetValue(MESSAGE, output.message);
    map.ExpectKeyGetValue(AGENTS, output.agents);
    map.ExpectKeyGetValue(ERROR_TRACKER, output.error_tracker);
  }
};

}  // namespace serializers
}  // namespace fetch
