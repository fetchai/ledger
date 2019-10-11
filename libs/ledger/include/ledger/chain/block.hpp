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
#include "core/byte_array/byte_array.hpp"
#include "core/digest.hpp"
#include "core/serializers/base_types.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"
#include "ledger/chain/transaction_layout.hpp"
#include "ledger/chain/transaction_layout_rpc_serializers.hpp"
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
  using Proof        = consensus::ProofOfWork;
  using Slice        = std::vector<TransactionLayout>;
  using Slices       = std::vector<Slice>;
  using DAGEpoch     = fetch::ledger::DAGEpoch;
  using Hash         = Digest;
  using Weight       = uint64_t;
  using BlockEntropy = beacon::BlockEntropy;
  using Identity     = crypto::Identity;
  using SystemClock  = moment::ClockPtr;

  Block();

  bool operator==(Block const &rhs) const;

  struct Body
  {
    Digest       hash;               ///< The hash of the block
    Digest       previous_hash;      ///< The hash of the previous block
    Digest       merkle_hash;        ///< The merkle state hash across all shards
    uint64_t     block_number{0};    ///< The height of the block from genesis
    Address      miner;              ///< The identity of the generated miner
    Identity     miner_id;           ///< The identity of the generated miner
    uint32_t     log2_num_lanes{0};  ///< The log2(number of lanes)
    Slices       slices;             ///< The slice lists
    DAGEpoch     dag_epoch;          ///< DAG epoch containing information on new dag_nodes
    uint64_t     timestamp{0u};      ///< The number of seconds elapsed since the Unix epoch
    BlockEntropy block_entropy;      ///< Entropy that determines miner priority for the next block
  };

  /// @name Block Contents
  /// @{
  Body body;  ///< The core fields that make up a block
  /// @}

  // Miners in qual must sign the block.
  Digest miner_signature;

  /// @name Proof of Work specifics
  /// @{
  uint64_t nonce{0};  ///< The nonce field associated with the proof
  Proof    proof;     ///< The consensus proof
  /// @}

  // TODO(HUT): This should be part of body since it's no longer going to be metadata
  Weight weight = 1;

  /// @name Metadata for block management
  /// @{
  Weight total_weight = 1;
  bool   is_loose     = false;
  /// @}

  // Helper functions
  std::size_t GetTransactionCount() const;
  void        UpdateDigest();
  void        UpdateTimestamp();
  SystemClock clock_ = moment::GetClock("block:body", moment::ClockType::SYSTEM);
};
}  // namespace ledger

namespace serializers {

template <class D>
struct MapSerializer<ledger::Block::Body, D>
  : MapSerializerTemplate<ledger::Block::Body, D,
                          SERIALIZED_STRUCT_FIELD(1, ledger::Block::Body::hash),
                          SERIALIZED_STRUCT_FIELD(2, ledger::Block::Body::previous_hash),
                          SERIALIZED_STRUCT_FIELD(3, ledger::Block::Body::merkle_hash),
                          SERIALIZED_STRUCT_FIELD(4, ledger::Block::Body::block_number),
                          SERIALIZED_STRUCT_FIELD(5, ledger::Block::Body::miner),
                          SERIALIZED_STRUCT_FIELD(6, ledger::Block::Body::miner_id),
                          SERIALIZED_STRUCT_FIELD(7, ledger::Block::Body::log2_num_lanes),
                          SERIALIZED_STRUCT_FIELD(8, ledger::Block::Body::slices),
                          SERIALIZED_STRUCT_FIELD(9, ledger::Block::Body::dag_epoch),
                          SERIALIZED_STRUCT_FIELD(10, ledger::Block::Body::timestamp),
                          SERIALIZED_STRUCT_FIELD(11, ledger::Block::Body::block_entropy)>
{
};

template <class D>
struct MapSerializer<ledger::Block, D>
  : MapSerializerTemplate<ledger::Block, D, SERIALIZED_STRUCT_FIELD(1, ledger::Block::body),
                          SERIALIZED_STRUCT_FIELD(2, ledger::Block::nonce),
                          SERIALIZED_STRUCT_FIELD(3, ledger::Block::proof),
                          SERIALIZED_STRUCT_FIELD(4, ledger::Block::weight),
                          SERIALIZED_STRUCT_FIELD(5, ledger::Block::total_weight),
                          SERIALIZED_STRUCT_FIELD(6, ledger::Block::miner_signature)>
{
};

}  // namespace serializers
}  // namespace fetch
