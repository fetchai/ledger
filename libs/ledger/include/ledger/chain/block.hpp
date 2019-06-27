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
#include "core/serializers/stl_types.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"
#include "ledger/chain/digest.hpp"
#include "ledger/chain/transaction_layout.hpp"

#include "ledger/dag/dag_epoch.hpp"

#include <memory>

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
  /// @}

  // Helper functions
  std::size_t GetTransactionCount() const;
  void        UpdateDigest();
  void        UpdateTimestamp();
};

/**
 * Serializer for the block body
 *
 * @tparam T The serializer type
 * @param serializer The reference to the serializer
 * @param body The reference to the body to be serialised
 */
template <typename T>
void Serialize(T &serializer, Block::Body const &body)
{
  serializer << body.hash << body.previous_hash << body.merkle_hash << body.block_number
             << body.miner << body.log2_num_lanes << body.slices << body.dag_epoch
             << body.timestamp;
}

/**
 * Deserializer for the block body
 *
 * @tparam T The serializer type
 * @param serializer The reference to the serializer
 * @param body The reference to the output body to be populated
 */
template <typename T>
void Deserialize(T &serializer, Block::Body &body)
{
  serializer >> body.hash >> body.previous_hash >> body.merkle_hash >> body.block_number >>
      body.miner >> body.log2_num_lanes >> body.slices >> body.dag_epoch >> body.timestamp;
}

/**
 * Serializer for the block
 *
 * @tparam T The serializer type
 * @param serializer The reference to hte serializer
 * @param block The reference to the block to be serialised
 */
template <typename T>
inline void Serialize(T &serializer, Block const &block)
{
  serializer << block.body << block.nonce << block.proof << block.weight << block.total_weight;
}

/**
 * Deserializer for the block
 *
 * @tparam T The serializer type
 * @param serializer The reference to the serializer
 * @param block The reference to the output block to be populated
 */
template <typename T>
inline void Deserialize(T &serializer, Block &block)
{
  serializer >> block.body >> block.nonce >> block.proof >> block.weight >> block.total_weight;
}

}  // namespace ledger
}  // namespace fetch
