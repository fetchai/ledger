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

#include "core/byte_array/byte_array.hpp"
#include "core/serializers/base_types.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"
#include "ledger/chain/digest.hpp"
#include "ledger/chain/transaction_layout.hpp"
#include "ledger/chain/transaction_layout_rpc_serializers.hpp"
#include "ledger/dag/dag_epoch.hpp"

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
  using Proof    = consensus::ProofOfWork;
  using Slice    = std::vector<TransactionLayout>;
  using Slices   = std::vector<Slice>;
  using DAGEpoch = fetch::ledger::DAGEpoch;

  Block();

  struct Body
  {
    Digest   hash;               ///< The hash of the block
    Digest   previous_hash;      ///< The hash of the previous block
    Digest   merkle_hash;        ///< The merkle state hash across all shards
    uint64_t block_number{0};    ///< The height of the block from genesis
    Address  miner;              ///< The identity of the generated miner
    uint32_t log2_num_lanes{0};  ///< The log2(number of lanes)
    Slices   slices;             ///< The slice lists
    DAGEpoch dag_epoch;          ///< DAG epoch containing information on new dag_nodes
    uint64_t timestamp{0u};      ///< The number of seconds elapsed since the Unix epoch
  };

  /// @name Block Contents
  /// @{
  Body body;  ///< The core fields that make up a block
  /// @}

  /// @name Proof of Work specifics
  /// @{
  uint64_t nonce{0};  ///< The nonce field associated with the proof
  Proof    proof;     ///< The consensus proof
  /// @}

  // TODO(HUT): This should be part of body since it's no longer going to be metadata
  uint64_t weight = 1;

  /// @name Metadata for block management
  /// @{
  uint64_t total_weight = 1;
  bool     is_loose     = false;
  uint64_t first_seen_timestamp{
      0u};  ///< Seconds since the block was first seen or created. Used to manage block interval
  /// @}

  // Helper functions
  std::size_t GetTransactionCount() const;
  void        UpdateDigest();
  void        UpdateTimestamp();
};
}  // namespace ledger

namespace serializers {

template <typename D>
struct MapSerializer<ledger::Block::Body, D>
{
public:
  using Type       = ledger::Block::Body;
  using DriverType = D;

  static uint8_t const HASH           = 1;
  static uint8_t const PREVIOUS_HASH  = 2;
  static uint8_t const MERKLE_HASH    = 3;
  static uint8_t const BLOCK_NUMBER   = 4;
  static uint8_t const MINER          = 5;
  static uint8_t const LOG2_NUM_LANES = 6;
  static uint8_t const SLICES         = 7;
  static uint8_t const DAG_EPOCH      = 8;
  static uint8_t const TIMESTAMP      = 9;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &body)
  {
    auto map = map_constructor(9);
    map.Append(HASH, body.hash);
    map.Append(PREVIOUS_HASH, body.previous_hash);
    map.Append(MERKLE_HASH, body.merkle_hash);
    map.Append(BLOCK_NUMBER, body.block_number);
    map.Append(MINER, body.miner);
    map.Append(LOG2_NUM_LANES, body.log2_num_lanes);
    map.Append(SLICES, body.slices);
    map.Append(DAG_EPOCH, body.dag_epoch);
    map.Append(TIMESTAMP, body.timestamp);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &body)
  {
    map.ExpectKeyGetValue(HASH, body.hash);
    map.ExpectKeyGetValue(PREVIOUS_HASH, body.previous_hash);
    map.ExpectKeyGetValue(MERKLE_HASH, body.merkle_hash);
    map.ExpectKeyGetValue(BLOCK_NUMBER, body.block_number);
    map.ExpectKeyGetValue(MINER, body.miner);
    map.ExpectKeyGetValue(LOG2_NUM_LANES, body.log2_num_lanes);
    map.ExpectKeyGetValue(SLICES, body.slices);
    map.ExpectKeyGetValue(DAG_EPOCH, body.dag_epoch);
    map.ExpectKeyGetValue(TIMESTAMP, body.timestamp);
  }
};

template <typename D>
struct MapSerializer<ledger::Block, D>
{
public:
  using Type       = ledger::Block;
  using DriverType = D;

  static uint8_t const BODY         = 1;
  static uint8_t const NONCE        = 2;
  static uint8_t const PROOF        = 3;
  static uint8_t const WEIGHT       = 4;
  static uint8_t const TOTAL_WEIGHT = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &block)
  {
    auto map = map_constructor(5);
    map.Append(BODY, block.body);
    map.Append(NONCE, block.nonce);
    map.Append(PROOF, block.proof);
    map.Append(WEIGHT, block.weight);
    map.Append(TOTAL_WEIGHT, block.total_weight);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &block)
  {
    map.ExpectKeyGetValue(BODY, block.body);
    map.ExpectKeyGetValue(NONCE, block.nonce);
    map.ExpectKeyGetValue(PROOF, block.proof);
    map.ExpectKeyGetValue(WEIGHT, block.weight);
    map.ExpectKeyGetValue(TOTAL_WEIGHT, block.total_weight);
  }
};

}  // namespace serializers
}  // namespace fetch
