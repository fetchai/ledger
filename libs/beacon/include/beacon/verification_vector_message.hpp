#pragma once
#include "beacon/beacon_manager.hpp"

#include <vector>

namespace fetch {
namespace beacon {
struct VerificationVectorMessage
{
  using BeaconManager = dkg::BeaconManager;

  uint64_t                                       round{0};
  std::vector<BeaconManager::VerificationVector> verification_vectors{};

  bool operator<(VerificationVectorMessage const &other) const
  {
    // Lower rounds come first, hence >
    return round > other.round;
  }
};
}  // namespace beacon

namespace serializers {

template <typename D>
struct MapSerializer<beacon::VerificationVectorMessage, D>
{
public:
  using Type       = beacon::VerificationVectorMessage;
  using DriverType = D;

  static uint8_t const ROUND                = 0;
  static uint8_t const VERIFICATION_VECTORS = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &vv)
  {
    auto map = map_constructor(2);

    map.Append(ROUND, vv.round);
    map.Append(VERIFICATION_VECTORS, vv.verification_vectors);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &vv)
  {
    map.ExpectKeyGetValue(ROUND, vv.round);
    map.ExpectKeyGetValue(VERIFICATION_VECTORS, vv.verification_vectors);
  }
};

}  // namespace serializers
}  // namespace fetch