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

#include "chain/address.hpp"
#include "chain/common_types.hpp"
#include "core/digest.hpp"
#include "crypto/fnv.hpp"
#include "crypto/identity.hpp"
#include "crypto/sha256.hpp"
#include "ledger/upow/synergetic_base_types.hpp"
#include "vectorise/uint/uint.hpp"

#include <limits>
#include <memory>

namespace fetch {
namespace chain {

class Address;

}  // namespace chain
namespace ledger {

class Work
{
public:
  using UInt256    = vectorise::UInt<256>;
  using BlockIndex = uint64_t;

  // Construction / Destruction
  Work() = default;
  explicit Work(BlockIndex block_index);
  Work(chain::Address address, crypto::Identity miner);
  Work(Work const &) = default;
  ~Work()            = default;

  // Getters
  chain::Address const &  address() const;
  crypto::Identity const &miner() const;
  UInt256 const &         nonce() const;
  WorkScore               score() const;
  BlockIndex              block_index() const;

  // Setters
  void UpdateAddress(chain::Address address);
  void UpdateIdentity(crypto::Identity const &identity);
  void UpdateScore(WorkScore score);
  void UpdateNonce(UInt256 const &nonce);

  // Actions
  UInt256 CreateHashedNonce() const;

private:
  chain::Address   contract_address_{};
  crypto::Identity miner_{};
  UInt256          nonce_{};
  WorkScore        score_{std::numeric_limits<WorkScore>::max()};
  BlockIndex       block_index_{0};

  template <typename T, typename S>
  friend struct serializers::MapSerializer;
};

using WorkPtr = std::shared_ptr<Work>;

}  // namespace ledger

namespace serializers {
template <typename D>
struct MapSerializer<ledger::Work, D>
{
public:
  using Type       = ledger::Work;
  using DriverType = D;

  static uint8_t const NONCE = 1;
  static uint8_t const SCORE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &work)
  {
    auto map = map_constructor(2);
    map.Append(NONCE, work.nonce_);
    map.Append(SCORE, work.score_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &work)
  {
    map.ExpectKeyGetValue(NONCE, work.nonce_);
    map.ExpectKeyGetValue(SCORE, work.score_);
  }
};
}  // namespace serializers

}  // namespace fetch
