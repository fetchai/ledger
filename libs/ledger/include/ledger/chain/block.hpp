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
  bool operator!=(Block const &rhs) const;

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
  uint64_t       chain_label{0};     ///< The label of a heaviest chain this block once belonged to

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
  : MapSerializerTemplate<ledger::Block, D,
	SERIALIZED_STRUCT_FIELD(1, ledger::Block::weight),
	SERIALIZED_STRUCT_FIELD(2, ledger::Block::total_weight),
	SERIALIZED_STRUCT_FIELD(3, ledger::Block::miner_signature),
	SERIALIZED_STRUCT_FIELD(4, ledger::Block::hash),
	SERIALIZED_STRUCT_FIELD(5, ledger::Block::previous_hash),
	SERIALIZED_STRUCT_FIELD(6, ledger::Block::merkle_hash),
	SERIALIZED_STRUCT_FIELD(7, ledger::Block::block_number),
	SERIALIZED_STRUCT_FIELD(8, ledger::Block::miner_id),
	SERIALIZED_STRUCT_FIELD(9, ledger::Block::log2_num_lanes),
	SERIALIZED_STRUCT_FIELD(10, ledger::Block::slices),
	SERIALIZED_STRUCT_FIELD(11, ledger::Block::dag_epoch),
	SERIALIZED_STRUCT_FIELD(12, ledger::Block::timestamp),
	SERIALIZED_STRUCT_FIELD(13, ledger::Block::block_entropy),
	SERIALIZED_STRUCT_FIELD(14, ledger::Block::miner)>
{};

}  // namespace serializers
}  // namespace fetch
