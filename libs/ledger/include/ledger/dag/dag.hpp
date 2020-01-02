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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/fnv.hpp"
#include "crypto/identity.hpp"
#include "crypto/sha256.hpp"
#include "ledger/dag/dag_epoch.hpp"
#include "ledger/dag/dag_interface.hpp"
#include "ledger/dag/dag_node.hpp"
#include "ledger/upow/work.hpp"
#include "storage/object_store.hpp"

#include <cstdint>
#include <deque>
#include <limits>
#include <list>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace fetch {
namespace crypto {
class Prover;
}

namespace ledger {

struct DAGTip
{
  using ConstByteArray = byte_array::ConstByteArray;

  DAGTip(DAGHash dag_node_ref, uint64_t epoch, uint64_t wei)
    : dag_node_reference{std::move(dag_node_ref)}
    , oldest_epoch_referenced{epoch}
    , weight{wei}
  {
    static uint64_t ids = 1;
    id                  = ids++;
  }

  DAGHash  dag_node_reference;  // Refers to a DagNode that has no references to it
  uint64_t oldest_epoch_referenced = 0;
  uint64_t weight                  = 0;
  uint64_t id                      = 0;
};

/**
 * DAG implementation.
 */
class DAG : public DAGInterface
{
private:
  using ByteArray       = byte_array::ByteArray;
  using ConstByteArray  = byte_array::ConstByteArray;
  using EpochStore      = fetch::storage::ObjectStore<DAGEpoch>;
  using DAGNodeStore    = fetch::storage::ObjectStore<DAGNode>;
  using NodeHash        = DAGHash;
  using EpochHash       = DAGHash;
  using EpochStackStore = fetch::storage::ObjectStore<EpochHash>;
  using DAGTipID        = uint64_t;
  using DAGTipPtr       = std::shared_ptr<DAGTip>;
  using DAGNodePtr      = std::shared_ptr<DAGNode>;
  using Mutex           = std::recursive_mutex;
  using CertificatePtr  = std::shared_ptr<crypto::Prover>;
  using DAGTypes        = DAGInterface::DAGTypes;

public:
  using MissingNodeHashes = std::set<NodeHash>;
  using MissingNodes      = std::set<DAGNode>;

  DAG() = delete;
  DAG(std::string db_name, bool load, CertificatePtr certificate);
  DAG(DAG const &rhs) = delete;
  DAG(DAG &&rhs)      = delete;
  DAG &operator=(DAG const &rhs) = delete;
  DAG &operator=(DAG &&rhs) = delete;

  static constexpr char const *LOGGING_NAME = "DAG";
  static const uint64_t        PARAMETER_REFERENCES_TO_BE_TIP =
      2;  // Must be > 1 as 1 reference signifies pointing at a DAGEpoch
  static const uint64_t EPOCH_VALIDITY_PERIOD = 2;
  static const uint64_t LOOSE_NODE_LIFETIME   = EPOCH_VALIDITY_PERIOD;
  static const uint64_t MAX_TIPS_IN_EPOCH     = 30;

  // Interface for adding what internally becomes a dag node. Easy to modify this interface
  // so do as you like with it Ed
  void AddTransaction(chain::Transaction const &tx, DAGTypes type) override;
  void AddWork(Work const &solution) override;
  void AddArbitrary(ConstByteArray const &payload) override;

  // Create an epoch based on the current DAG (not committal)
  DAGEpoch CreateEpoch(uint64_t block_number) override;

  // Commit the state of the DAG as this node believes it (using an epoch)
  bool CommitEpoch(DAGEpoch new_epoch) override;

  // Revert to a previous/forward epoch
  bool RevertToEpoch(uint64_t epoch_bn_to_revert) override;

  uint64_t CurrentEpoch() const override;

  bool HasEpoch(EpochHash const &hash) override;

  // Make sure that the dag has all nodes for a certain epoch
  bool SatisfyEpoch(DAGEpoch const &epoch) override;

  std::vector<DAGNode> GetLatest(bool previous_epoch_only) override;

  ///////////////////////////////////////
  // Fns used for syncing

  // Recently added DAG nodes by this miner (not seen by network yet)
  std::vector<DAGNode> GetRecentlyAdded() override;

  // DAG node hashes this miner knows should exist
  MissingNodeHashes GetRecentlyMissing() override;

  bool GetDAGNode(DAGHash const &hash, DAGNode &node) override;
  bool GetWork(DAGHash const &hash, Work &work) override;

  // TXs and epochs will be added here when they are broadcast in
  bool AddDAGNode(DAGNode node) override;

private:
  // Long term storage
  uint64_t             most_recent_epoch_ = 0;
  DAGEpoch             previous_epoch_;   // Most recent epoch, not in deque for convenience
  std::deque<DAGEpoch> previous_epochs_;  // N - 1 still relevant epochs
  EpochStackStore      epochs_;  // Past less-relevant epochs as a stack (key = index, value = hash)
  EpochStore all_stored_epochs_;  // All epochs, including from non-winning forks (key = epoch hash,
                                  // val = epoch)
  DAGNodeStore finalised_dag_nodes_;  // Once an epoch arrives, all dag nodes in between go here

  // clang-format off
  // volatile state
  std::unordered_map<DAGTipID, DAGTipPtr>               all_tips_;  // All tips are here
  std::unordered_map<NodeHash, DAGTipPtr>               tips_;  // look up tips of the dag pointing at a certain node hash
  std::unordered_map<NodeHash, DAGNodePtr>              node_pool_;  // dag nodes that are not finalised but are still valid
  std::unordered_map<NodeHash, DAGNodePtr>              loose_nodes_;  // nodes that are missing one or more references (waiting on NodeHash)
  std::unordered_map<NodeHash, std::vector<DAGNodePtr>> loose_nodes_lookup_;  // nodes that are missing one or more references (waiting on NodeHash)
  // clang-format on

  // TODO(1642): loose nodes management scheme
  // std::unordered_map<NodeHash, uint64_t> loose_nodes_ttl_;

  // Used for sync purposes
  std::vector<DAGNode> recently_added_;  // nodes that have been recently added
  std::set<NodeHash>   missing_;         // node hashes that we know are missing

  // Internal functions don't need locking and can recursively call themselves etc.
  bool       PushInternal(DAGNodePtr const &node);
  bool       AlreadySeenInternal(DAGNodePtr const &node) const;
  bool       TooOldInternal(uint64_t oldest_reference) const;
  bool       IsLooseInternal(DAGNodePtr const &node) const;
  void       SetReferencesInternal(DAGNodePtr const &node);
  void       AdvanceTipsInternal(DAGNodePtr const &node);
  bool       HashInPrevEpochsInternal(DAGHash const &hash) const;
  void       AddLooseNodeInternal(DAGNodePtr const &node);
  void       HealLooseBlocksInternal(DAGHash const &added_hash);
  void       UpdateStaleTipsInternal();
  bool       NodeInvalidInternal(DAGNodePtr const &node);
  DAGNodePtr GetDAGNodeInternal(DAGHash const &hash, bool including_loose,
                                bool &was_loose);  // const
  void       TraverseFromTips(std::set<DAGHash> const &            tip_hashes,
                              std::function<void(NodeHash)> const &on_node,
                              std::function<bool(NodeHash)> const &terminating_condition);
  bool       GetEpochFromStorage(std::string const &identifier, DAGEpoch &epoch);
  bool       SetEpochInStorage(std::string const & /*unused*/, DAGEpoch const &epoch, bool is_head);
  void       Flush();

  void DeleteTip(DAGTipID tip_id);
  void DeleteTip(NodeHash const &hash);

  std::string    db_name_;
  CertificatePtr certificate_;
  mutable Mutex  mutex_;
};

}  // namespace ledger
}  // namespace fetch
