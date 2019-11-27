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

#include "beacon/block_entropy.hpp"
#include "chain/address.hpp"
#include "chain/transaction_layout.hpp"
#include "chain/transaction_layout_rpc_serializers.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/digest.hpp"
#include "core/serializers/base_types.hpp"
#include "ledger/dag/dag_epoch.hpp"
#include "moment/clocks.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace fetch {
namespace ledger {

/**
 * The block class constitutes the complete node that forms the main chain. The block class is
 * split into two levels. The body, which is consensus agnostic and the main block which has
 * consensus specific details.
 */
class Block
{
public:
  using Slice        = std::vector<chain::TransactionLayout>;
  using Slices       = std::vector<Slice>;
  using DAGEpoch     = fetch::ledger::DAGEpoch;
  using Hash         = Digest;
  using Weight       = uint64_t;
  using BlockEntropy = beacon::BlockEntropy;
  using Identity     = crypto::Identity;
  using SystemClock  = moment::ClockPtr;
  using Index        = uint64_t;

  Block() = default;

  bool operator==(Block const &rhs) const;

  // Block core information
  Digest         hash;               ///< The hash of the block
  Digest         previous_hash;      ///< The hash of the previous block
  Digest         merkle_hash;        ///< The merkle state hash across all shards
  uint64_t       block_number{0};    ///< The height of the block from genesis
  chain::Address miner;              ///< The identity of the generated miner
  Identity       miner_id;           ///< The identity of the generated miner
  uint32_t       log2_num_lanes{0};  ///< The log2(number of lanes)
  Slices         slices;             ///< The slice lists
  DAGEpoch       dag_epoch;          ///< DAG epoch containing information on new dag_nodes
  uint64_t       timestamp{0u};      ///< The number of seconds elapsed since the Unix epoch
  BlockEntropy   block_entropy;      ///< Entropy that determines miner priority for the next block
  Weight         weight = 1;         ///< Block weight

  // The qual miner must sign the block
  Digest miner_signature;

  /// @name Metadata for block management (not serialized)
  /// @{
  Weight total_weight = 1;
  bool   is_loose     = false;
  /// @}

  // Helper functions
  std::size_t GetTransactionCount() const;
  void        UpdateDigest();
  void        UpdateTimestamp();
  bool        IsGenesis() const;

private:
  SystemClock clock_ = moment::GetClock("block:body", moment::ClockType::SYSTEM);
};
}  // namespace ledger

namespace serializers {

template <typename D>
struct MapSerializer<ledger::Block, D>
{
public:
  using Type       = ledger::Block;
  using DriverType = D;

  static uint8_t const WEIGHT          = 1;
  static uint8_t const TOTAL_WEIGHT    = 2;
  static uint8_t const MINER_SIGNATURE = 3;
  static uint8_t const HASH            = 4;
  static uint8_t const PREVIOUS_HASH   = 5;
  static uint8_t const MERKLE_HASH     = 6;
  static uint8_t const BLOCK_NUMBER    = 7;
  static uint8_t const MINER_ID        = 8;
  static uint8_t const LOG2_NUM_LANES  = 9;
  static uint8_t const SLICES          = 10;
  static uint8_t const DAG_EPOCH       = 11;
  static uint8_t const TIMESTAMP       = 12;
  static uint8_t const ENTROPY         = 13;
  static uint8_t const MINER           = 14;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &block)
  {
    auto map = map_constructor(14);
    map.Append(WEIGHT, block.weight);
    map.Append(TOTAL_WEIGHT, block.total_weight);
    map.Append(MINER_SIGNATURE, block.miner_signature);
    map.Append(HASH, block.hash);
    map.Append(PREVIOUS_HASH, block.previous_hash);
    map.Append(MERKLE_HASH, block.merkle_hash);
    map.Append(BLOCK_NUMBER, block.block_number);
    map.Append(MINER_ID, block.miner_id);
    map.Append(LOG2_NUM_LANES, block.log2_num_lanes);
    map.Append(SLICES, block.slices);
    map.Append(DAG_EPOCH, block.dag_epoch);
    map.Append(TIMESTAMP, block.timestamp);
    map.Append(ENTROPY, block.block_entropy);
    map.Append(MINER, block.miner);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &block)
  {
    map.ExpectKeyGetValue(WEIGHT, block.weight);
    map.ExpectKeyGetValue(TOTAL_WEIGHT, block.total_weight);
    map.ExpectKeyGetValue(MINER_SIGNATURE, block.miner_signature);
    map.ExpectKeyGetValue(HASH, block.hash);
    map.ExpectKeyGetValue(PREVIOUS_HASH, block.previous_hash);
    map.ExpectKeyGetValue(MERKLE_HASH, block.merkle_hash);
    map.ExpectKeyGetValue(BLOCK_NUMBER, block.block_number);
    map.ExpectKeyGetValue(MINER_ID, block.miner_id);
    map.ExpectKeyGetValue(LOG2_NUM_LANES, block.log2_num_lanes);
    map.ExpectKeyGetValue(SLICES, block.slices);
    map.ExpectKeyGetValue(DAG_EPOCH, block.dag_epoch);
    map.ExpectKeyGetValue(TIMESTAMP, block.timestamp);
    map.ExpectKeyGetValue(ENTROPY, block.block_entropy);
    map.ExpectKeyGetValue(MINER, block.miner);
  }
};

}  // namespace serializers
}  // namespace fetch
