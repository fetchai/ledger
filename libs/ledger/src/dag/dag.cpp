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

#include <utility>

#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/dag/dag.hpp"
#include "ledger/dag/dag_node.hpp"

#include <random>

using namespace fetch::ledger;

DAG::DAG(std::string db_name, bool load, CertificatePtr certificate)
  : db_name_{std::move(db_name)}
  , certificate_{std::move(certificate)}
{

  // Fallback is to reset everything
  auto CreateCleanState = [this]() {
    epochs_.New(db_name_ + "_epochs.db", db_name_ + "_epochs.index.db");
    all_stored_epochs_.New(db_name_ + "_all_epochs.db", db_name_ + "_all_epochs.index.db");
    finalised_dag_nodes_.New(db_name_ + "_fin_nodes.db", db_name_ + "_fin_nodes.index.db");

    most_recent_epoch_ = 0;
    previous_epochs_.clear();
    all_tips_.clear();
    tips_.clear();
    node_pool_.clear();
    loose_nodes_.clear();
    loose_nodes_lookup_.clear();
    recently_added_.clear();
    missing_.clear();

    // for epoch 0 - this should always be empty
    previous_epoch_ = DAGEpoch{};
    previous_epoch_.Finalise();
  };

  // If everything is in order we can recreate our epoch state by pushing from the store into our
  // deque
  auto RecoverToEpoch = [this](uint64_t block_number) -> bool {
    DAGEpoch recover_epoch;

    // Get head
    if (!GetEpochFromStorage(std::to_string(block_number), recover_epoch))
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "No head found when loading epoch store!");
      return false;
    }

    // Push head - N until the memory deque is full
    while (previous_epochs_.size() < EPOCH_VALIDITY_PERIOD)
    {
      previous_epochs_.push_back(recover_epoch);

      if (recover_epoch.block_number == 0)
      {
        break;
      }

      uint64_t next_epoch = recover_epoch.block_number - 1;

      if (!GetEpochFromStorage(std::to_string(next_epoch), recover_epoch))
      {
        FETCH_LOG_ERROR(
            LOGGING_NAME,
            "Epoch not found when traversing/recovering from file. Index: ", next_epoch);
        return false;
      }
    }

    assert(previous_epochs_.size() > 0);

    // Sanity check
    for (auto const &epoch : previous_epochs_)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Recovered epoch: ", epoch.block_number);
    }

    // mend
    previous_epoch_ = previous_epochs_.front();
    previous_epochs_.pop_front();
    most_recent_epoch_ = previous_epoch_.block_number;

    return true;
  };

  bool success_loading = true;

  if (load)
  {
    // Attempt to load state
    epochs_.Load(db_name_ + "_epochs.db", db_name_ + "_epochs.index.db");
    all_stored_epochs_.Load(db_name_ + "_all_epochs.db", db_name_ + "_all_epochs.index.db");
    finalised_dag_nodes_.Load(db_name_ + "_fin_nodes.db", db_name_ + "_fin_nodes.index.db");

    DAGEpoch recover_head;

    if (!GetEpochFromStorage("HEAD", recover_head))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "No head found when loading epoch store!");
      success_loading = false;
    }

    if (success_loading)
    {
      success_loading = RecoverToEpoch(recover_head.block_number);
    }
  }

  if (!load || !success_loading)
  {
    CreateCleanState();
  }
}

std::vector<DAGNode> DAG::GetLatest(bool previous_epoch_only)
{
  std::vector<DAGNode> ret;
  FETCH_LOCK(mutex_);

  for (auto const &node_hash : previous_epoch_.all_nodes)
  {
    bool dummy;
    auto node_internal = GetDAGNodeInternal(node_hash, false, dummy);

    if (!node_internal)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Node referenced in previous epoch not found!");
    }

    ret.push_back(*node_internal);
  }

  if (!previous_epoch_only)
  {
    for (auto const &epoch : previous_epochs_)
    {
      for (auto const &node_hash : epoch.all_nodes)
      {
        bool dummy;
        auto node_internal = GetDAGNodeInternal(node_hash, false, dummy);

        if (!node_internal)
        {
          FETCH_LOG_ERROR(LOGGING_NAME, "Node referenced in previous epoch not found!");
        }

        ret.push_back(*node_internal);
      }
    }
  }

  return ret;
}

void DAG::AddTransaction(Transaction const &tx, DAGTypes type)
{
  if (type != DAGTypes::DATA)
  {
    return;
  }

  FETCH_LOCK(mutex_);

  // Create a new dag node containing this data
  DAGNodePtr new_node = std::make_shared<DAGNode>();

  new_node->type = DAGNode::DATA;
  new_node->SetContents(tx);
  new_node->contract_digest = tx.contract_digest().address();
  new_node->contents        = tx.data();
  SetReferencesInternal(new_node);

  new_node->identity = certificate_->identity();
  new_node->Finalise();
  new_node->signature = certificate_->Sign(new_node->hash);

  PushInternal(new_node);
  recently_added_.push_back(*new_node);
}

void DAG::AddWork(Work const &solution)
{
  FETCH_LOCK(mutex_);

  // Create a new dag node containing this data
  DAGNodePtr new_node = std::make_shared<DAGNode>();

  serializers::ByteArrayBuffer buffer;
  buffer << solution;

  new_node->type            = DAGNode::WORK;
  new_node->contract_digest = solution.contract_digest();
  new_node->identity        = solution.miner();
  new_node->contents        = buffer.data();

  SetReferencesInternal(new_node);

  new_node->identity = certificate_->identity();
  new_node->Finalise();
  new_node->signature = certificate_->Sign(new_node->hash);

  PushInternal(new_node);
  recently_added_.push_back(*new_node);
}

void DAG::AddArbitrary(ConstByteArray const &payload)
{
  FETCH_LOCK(mutex_);

  // Create a new dag node containing this data
  DAGNodePtr new_node = std::make_shared<DAGNode>();

  new_node->type     = DAGNode::ARBITRARY;
  new_node->contents = payload;
  SetReferencesInternal(new_node);
  new_node->identity = certificate_->identity();
  new_node->Finalise();
  new_node->signature = certificate_->Sign(new_node->hash);

  PushInternal(new_node);
  recently_added_.push_back(*new_node);
}

// Get as many references as required for the node, when adding. DAG nodes or epoch hashes are
// valid, but don't validate dag nodes already finalised since that adds little information
void DAG::SetReferencesInternal(DAGNodePtr node)
{
  // We are going to fill the prev references, epoch and weight
  auto &    prevs        = node->previous;
  uint64_t &oldest_epoch = node->oldest_epoch_referenced;
  uint64_t &wei          = node->weight;

  // Corner case - not enough nodes to reference - just point a single reference to the prev epoch
  if (node_pool_.size() < PARAMETER_REFERENCES_TO_BE_TIP)
  {
    prevs.push_back(previous_epoch_.hash);
    oldest_epoch = most_recent_epoch_;
    wei          = 0;
    return;
  }

  // Enough tips to randomly choose to to reference
  if (all_tips_.size() >= PARAMETER_REFERENCES_TO_BE_TIP)
  {
    std::vector<uint64_t> dag_ids;

    for (auto it = all_tips_.begin(); it != all_tips_.end(); ++it)
    {
      dag_ids.push_back(it->first);
    }

    // TODO (1223) probably not the best place to instantiate/store the random device
    std::random_device rd;
    std::mt19937       g(rd());
    std::shuffle(dag_ids.begin(), dag_ids.end(), g);

    while (prevs.size() < PARAMETER_REFERENCES_TO_BE_TIP && !dag_ids.empty())
    {
      // extract the current tip id from the vector
      auto const current_tip_id = dag_ids.back();
      dag_ids.pop_back();

      auto rnd_tip_ref = all_tips_.at(current_tip_id);
      prevs.push_back(rnd_tip_ref->dag_node_reference);

      if (wei <= rnd_tip_ref->weight)
      {
        wei = rnd_tip_ref->weight + 1;
      }

      if (oldest_epoch > rnd_tip_ref->oldest_epoch_referenced)
      {
        oldest_epoch = rnd_tip_ref->oldest_epoch_referenced;
      }

      DeleteTip(current_tip_id);
    }
  }
  else
  {
    // In the case there are not enough tips to reference, choose non-tip nodes
    // TODO(HUT): inefficient, but node pool is almost certainly small
    std::unordered_map<NodeHash, DAGNodePtr> node_pool_copy = node_pool_;

    while (prevs.size() < PARAMETER_REFERENCES_TO_BE_TIP && !node_pool_copy.empty())
    {
      auto &node = (node_pool_copy.begin())->second;

      prevs.push_back(node->hash);

      if (wei <= node->weight)
      {
        wei = node->weight + 1;
      }

      if (oldest_epoch > node->oldest_epoch_referenced)
      {
        oldest_epoch = node->oldest_epoch_referenced;
      }

      node_pool_copy.erase(node_pool_copy.begin());
    }
  }
}

bool DAG::AddDAGNode(DAGNode node)
{
  assert(node.hash.size() > 0);
  FETCH_LOCK(mutex_);

  missing_.erase(node.hash);
  bool success = PushInternal(std::make_shared<DAGNode>(node));

  return success;
}

std::vector<DAGNode> DAG::GetRecentlyAdded()
{
  FETCH_LOCK(mutex_);
  std::vector<DAGNode> ret = std::move(recently_added_);
  recently_added_.clear();
  return ret;
}

std::set<fetch::byte_array::ConstByteArray> DAG::GetRecentlyMissing()
{
  FETCH_LOCK(mutex_);
  std::set<NodeHash> ret = missing_;
  return ret;
}

// Node is loose when not all references are found in the last N block periods
bool DAG::IsLooseInternal(DAGNodePtr node)
{
  for (auto const &dag_node_prev : node->previous)
  {
    if (node_pool_.find(dag_node_prev) == node_pool_.end() &&
        !HashInPrevEpochsInternal(dag_node_prev))
    {
      return true;
    }
  }

  return false;
}

// Add this node as loose - one or more entries in loose_nodes[missing_hash]
void DAG::AddLooseNodeInternal(DAGNodePtr node)
{
  for (auto const &dag_node_prev : node->previous)
  {
    if (node_pool_.find(dag_node_prev) == node_pool_.end() &&
        !HashInPrevEpochsInternal(dag_node_prev))
    {
      loose_nodes_lookup_[dag_node_prev].push_back(node);
      loose_nodes_[node->hash] = node;
    }
  }
}

// Check whether the hash refers to anything considered valid that's not in the node pool
bool DAG::HashInPrevEpochsInternal(ConstByteArray hash)
{
  // Check if hash is a node in epoch
  if (previous_epoch_.Contains(hash))
  {
    return true;
  }

  // check if hash is epoch/prev epochs
  if (hash == previous_epoch_.hash)
  {
    return true;
  }

  for (auto const &epoch : previous_epochs_)
  {
    if (hash == epoch.hash || epoch.Contains(hash))
    {
      return true;
    }
  }

  return false;
}

// check whether the node has already been added for this period
bool DAG::AlreadySeenInternal(DAGNodePtr node)
{
  if (node_pool_.find(node->hash) != node_pool_.end() || HashInPrevEpochsInternal(node->hash))
  {
    return true;
  }

  return false;
}

bool DAG::TooOldInternal(uint64_t oldest_reference)
{
  if ((oldest_reference + EPOCH_VALIDITY_PERIOD) <= most_recent_epoch_)
  {
    return true;
  }

  return false;
}

bool DAG::GetDAGNode(ConstByteArray const &hash, DAGNode &node)
{
  FETCH_LOCK(mutex_);
  bool dummy;
  auto ret = GetDAGNodeInternal(hash, true, dummy);

  if (ret)
  {
    node = *ret;
    FETCH_LOG_DEBUG(LOGGING_NAME, "Request for dag node", hash.ToBase64(), " win");
    FETCH_LOG_DEBUG(LOGGING_NAME, "DAG node hash: ", node.hash.ToBase64());
    return true;
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Request for dag node", hash.ToBase64(), " lose");
    return false;
  }
}

bool DAG::GetWork(ConstByteArray const &hash, Work &work)
{
  FETCH_LOCK(mutex_);
  bool success{false};

  // lookup the DAG node in question
  DAGNode node;
  if (GetDAGNode(hash, node))
  {
    try
    {
      serializers::ByteArrayBuffer buffer{node.contents};
      buffer >> work;

      // add fields normally not serialised
      work.UpdateDigest(node.contract_digest);
      work.UpdateIdentity(node.identity);

      // debug
      // work.originating_node = hash;

      success = true;
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to deserialise the work from node: ", ex.what());
    }
  }

  return success;
}

std::shared_ptr<DAGNode> DAG::GetDAGNodeInternal(ConstByteArray hash, bool including_loose,
                                                 bool &was_loose)
{
  // Find in node pool
  auto it2 = node_pool_.find(hash);
  if (it2 != node_pool_.end())
  {
    return node_pool_[hash];
  }

  was_loose = false;

  // If we are also looking for loose blocks
  if (including_loose)
  {
    auto it = loose_nodes_.find(hash);
    if (it != loose_nodes_.end())
    {
      was_loose = true;
      return loose_nodes_[hash];
    }
  }

  // Find in long term storage
  DAGNodePtr ret = std::make_shared<DAGNode>();

  if (finalised_dag_nodes_.Get(storage::ResourceID(hash), *ret))
  {
    return ret;
  }

  return {};
}

// Add a dag node
bool DAG::PushInternal(DAGNodePtr node)
{
  assert(node);

  // First check if we have already seen this node
  if (AlreadySeenInternal(node))
  {
    return false;
  }

  // Check if node refers too far back in the dag to be considered valid
  if (TooOldInternal(node->oldest_epoch_referenced))
  {
    return false;
  }

  // Check if loose
  if (IsLooseInternal(node))
  {
    AddLooseNodeInternal(node);

    // There is now a chance that adding this node completed some loose nodes.
    HealLooseBlocksInternal(node->hash);

    return true;
  }

  // At this point the node is suitable for addition and needs to be checked
  if (NodeInvalidInternal(node))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "is INVALID!");
    return false;
  }

  // Add to node pool, update any tips that advance due to this
  node_pool_[node->hash] = node;
  AdvanceTipsInternal(node);

  // There is now a chance that adding this node completed some loose nodes.
  HealLooseBlocksInternal(node->hash);

  return true;
}

void DAG::HealLooseBlocksInternal(ConstByteArray added_hash)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Healing: ", added_hash.ToBase64());

  while (true)
  {
    // 'it' will be a vector of dag nodes that were waiting for this hash
    auto it = loose_nodes_lookup_.find(added_hash);

    if (it == loose_nodes_lookup_.end())
    {
      break;
    }

    auto &loose_nodes = (*it).second;

    if (loose_nodes.empty())
    {
      loose_nodes_lookup_.erase(it);
      break;
    }

    // remove dag node from this vector
    DAGNodePtr possibly_non_loose = loose_nodes.back();
    loose_nodes.pop_back();

    // dag node could have other hash -> loose reference in loose_nodes_lookup_.
    // if it doesn't, all hashes it was looking for have been seen. Either
    // it gets added now or it's discarded
    bool hash_still_in_loose = false;

    for (auto const &dag_node_prev : possibly_non_loose->previous)
    {
      if (loose_nodes_lookup_.find(dag_node_prev) != loose_nodes_lookup_.end())
      {
        hash_still_in_loose = true;
        break;
      }
    }

    if (hash_still_in_loose)
    {
      continue;
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "Healing loose node: ", possibly_non_loose->hash.ToBase64());

    // At this point, the node is either added or dropped
    assert(loose_nodes_.find(possibly_non_loose->hash) != loose_nodes_.end());
    loose_nodes_.erase(possibly_non_loose->hash);

    if (!IsLooseInternal(possibly_non_loose))
    {
      PushInternal(possibly_non_loose);
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Dag structure keeping track of missing nodes might be invalid");
    }
  }
}

// Create an epoch given our current tips (doesn't advance the dag)
DAGEpoch DAG::CreateEpoch(uint64_t block_number)
{
  FETCH_LOCK(mutex_);
  DAGEpoch ret;
  ret.block_number = block_number;

  if (block_number == 0)
  {
    ret.Finalise();
    return ret;
  }

  if (block_number != most_recent_epoch_ + 1)
  {
    throw std::runtime_error("Attempt to create an epoch from a desynchronised DAG");
  }

  std::set<ConstByteArray> tips_to_add;

  auto it = all_tips_.begin();

  // Assume here that our own tips are valid (and non circular!)
  while (tips_to_add.size() < MAX_TIPS_IN_EPOCH && it != all_tips_.end())
  {
    auto tip_ptr = it->second;
    tips_to_add.insert(tip_ptr->dag_node_reference);
    it++;
  }

  // Find all un-finalised nodes given tips in epoch
  std::set<ConstByteArray> all_nodes_to_add;

  auto on_node = [&all_nodes_to_add](NodeHash current) { all_nodes_to_add.insert(current); };

  auto terminating_condition = [&all_nodes_to_add](NodeHash current) -> bool {
    // Terminate when already seen node for efficiency reasons
    if (all_nodes_to_add.find(current) != all_nodes_to_add.end())
    {
      return true;
    }
    return false;
  };

  // Traverse down from the tips (for unaccounted for dagnodes), adding
  // tips to all_nodes_to_add
  TraverseFromTips(tips_to_add, on_node, terminating_condition);

  ret.tips      = tips_to_add;
  ret.all_nodes = all_nodes_to_add;

  DAGNodePtr dag_node_to_add;

  // Fill the TX field
  // TODO(HUT): this needs ordering
  for (auto const &dag_node_hash : all_nodes_to_add)
  {
    dag_node_to_add = node_pool_.at(dag_node_hash);

    switch (dag_node_to_add->type)
    {
    case DAGNode::WORK:
      ret.solution_nodes.insert(dag_node_to_add->hash);
      break;
    case DAGNode::DATA:
      ret.data_nodes.insert(dag_node_to_add->hash);
      break;
    case DAGNode::ARBITRARY:
      break;
    case DAGNode::GENESIS:
      break;
    }
  }

  ret.Finalise();

  return ret;
}

// Commit to a valid epoch - note it is important that this has been validated in some way first
// TODO(HUT): const this.
bool DAG::CommitEpoch(DAGEpoch new_epoch)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Committing epoch: ", new_epoch.block_number,
                 " Nodes: ", new_epoch.all_nodes.size());
  FETCH_LOCK(mutex_);

  if (new_epoch.block_number == 0)
  {
    return false;
  }

  if (new_epoch.block_number != most_recent_epoch_ + 1)
  {
    FETCH_LOG_WARN(
        LOGGING_NAME,
        "Attempt to commit a bad epoch: not an increment on the current DAG epoch! Current: ",
        most_recent_epoch_, " attempted: ", new_epoch.block_number);
    return false;
  }

  for (auto const &node_hash : new_epoch.all_nodes)
  {
    // Remove all TXs from the active node pool that are referenced in this epoch
    // also remove tips that refer to nodes in this epoch
    // Note that unaffected tips will remain valid but will have less elements

    // move finalised nodes into long term storage
    auto it_node_to_rmv = node_pool_.find(node_hash);
    if (it_node_to_rmv != node_pool_.end())
    {
      DAGNodePtr &node_to_remove = it_node_to_rmv->second;
      finalised_dag_nodes_.Set(storage::ResourceID(node_to_remove->hash), *node_to_remove);
      node_pool_.erase(it_node_to_rmv);
    }
    else if (loose_nodes_.find(node_hash) != loose_nodes_.end())
    {
      auto        loose_node_it  = loose_nodes_.find(node_hash);
      DAGNodePtr &node_to_remove = loose_node_it->second;
      finalised_dag_nodes_.Set(storage::ResourceID(node_to_remove->hash), *node_to_remove);
      loose_nodes_.erase(loose_node_it);

      // TODO(HUT): check that there are no references in loose_nodes_lookup_
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Epoch references a DAG node that is not in our node pool!");
    }

    // remove tips that reference this dag node directly
    auto it_tip_to_rmv = tips_.find(node_hash);
    if (it_tip_to_rmv != tips_.end())
    {
      DeleteTip(node_hash);
    }
  }

  // push back current epoch
  {
    previous_epochs_.push_back(previous_epoch_);
    previous_epoch_ = new_epoch;

    if (previous_epochs_.size() > (EPOCH_VALIDITY_PERIOD - 1))
    {
      auto &front_epoch = previous_epochs_.front();
      assert(front_epoch.hash.size() > 0);
      SetEpochInStorage(std::to_string(front_epoch.block_number), front_epoch, true);
      previous_epochs_.pop_front();
    }
  }

  most_recent_epoch_++;

  // Some tips will now refer to parts of the dag that are too old. Need to refresh these
  UpdateStaleTipsInternal();

  // Some nodes will have been looking for this hash, heal these
  HealLooseBlocksInternal(new_epoch.hash);

  Flush();

  return true;
}

void DAG::Flush()
{
  epochs_.Flush(false);
  all_stored_epochs_.Flush(false);
  finalised_dag_nodes_.Flush(false);
}

void DAG::TraverseFromTips(std::set<ConstByteArray> const &tip_hashes,
                           std::function<void(NodeHash)>   on_node,
                           std::function<bool(NodeHash)>   terminating_condition)
{
  for (auto it = tip_hashes.begin(); it != tip_hashes.end(); ++it)
  {
    if (node_pool_.find(*it) == node_pool_.end())
    {
      throw std::runtime_error("Tip found in DAG that refers nowhere");
    }

    NodeHash              start = node_pool_[*it]->hash;
    DAGNodePtr            dag_node_to_add;
    std::vector<uint64_t> switch_choices{0};
    std::vector<NodeHash> switch_hashes{start};

    if (HashInPrevEpochsInternal(start))
    {
      throw std::runtime_error("Tip found in DAG that refers to something finalised");
    }

    // Depth first search of the dag until reaching a finalised node/hash
    // Warning: If the dag is circular this will not terminate
    while (switch_choices.size() > 0)
    {
      start = switch_hashes.back();  // Hash under evaluation

      // Hash 'terminates' - refers to already used hash
      if (HashInPrevEpochsInternal(start))
      {
        switch_choices.pop_back();
        switch_hashes.pop_back();
        continue;
      }

      // Check user supplied terminating condition (usually at least this would be that
      // the hash and by definition subgraph have already been added)
      if (terminating_condition(start))
      {
        switch_choices.pop_back();
        switch_hashes.pop_back();
        continue;
      }

      // Attempt to take the path specified by switch choices, else add it
      dag_node_to_add = node_pool_.at(start);

      // If all paths are exhausted for this hash
      if (switch_choices.back() == dag_node_to_add->previous.size())
      {
        on_node(start);
        /* all_nodes_to_add.insert(start); */
        switch_choices.pop_back();
        switch_hashes.pop_back();
        continue;
      }

      // There are unexplored paths still - explore one of these
      start = dag_node_to_add->previous[switch_choices.back()];
      switch_hashes.push_back(start);

      switch_choices[switch_choices.size() - 1]++;
      switch_choices.push_back(0);
    }
  }
}

// TODO(HUT): this.
void DAG::UpdateStaleTipsInternal()
{
  // for tips that now contain a subgraph that is out of scope (not in recent epochs),
  // delete, traverse these and create new tips that are in scope
  std::set<NodeHash>
                     stale_tips_to_delete;  // Tips that somewhere in their dag refer to an old dag node
  std::set<NodeHash> stale_nodes;           // Nodes that refer somewhere to an old dag node
  std::set<NodeHash> new_tip_locations;  // Nodes that were referenced by a now stale dag tip, but
                                         // are themselves still ok

  for (auto const &tip : all_tips_)
  {
    DAGTipPtr ref = tip.second;
    if (TooOldInternal(ref->oldest_epoch_referenced))
    {
      stale_tips_to_delete.insert(ref->dag_node_reference);
    }
  }

  auto on_node = [&stale_nodes](NodeHash current) { stale_nodes.insert(current); };

  auto terminating_condition = [this, &stale_nodes, &new_tip_locations](NodeHash current) -> bool {
    // Terminate when already seen node for efficiency reasons
    if (stale_nodes.find(current) != stale_nodes.end())
    {
      return true;
    }

    // Terminate when this node is healthy - this is a new tip
    DAGNodePtr dag_node_to_check;

    if (node_pool_.find(current) != node_pool_.end())
    {
      dag_node_to_check = node_pool_[current];

      if (!TooOldInternal(dag_node_to_check->oldest_epoch_referenced))
      {
        new_tip_locations.insert(current);
        return true;
      }
    }

    return false;
  };

  TraverseFromTips(stale_tips_to_delete, on_node, terminating_condition);

  // Cleanup - remove old tips and nodes - note that stale_tips_to_delete is a subset of stale_nodes
  for (auto const &stale_node_hash : stale_nodes)
  {
    auto it = tips_.find(stale_node_hash);

    if (it != tips_.end())
    {
      DeleteTip(it->second->id);
    }

    node_pool_.erase(stale_node_hash);
  }

  // Update : new tips need to be created
  for (auto const &new_tip_loc : new_tip_locations)
  {
    DAGNodePtr node = node_pool_.at(new_tip_loc);

    DAGTipPtr new_dag_tip =
        std::make_shared<DAGTip>(node->hash, node->oldest_epoch_referenced, node->weight);

    tips_[node->hash]          = new_dag_tip;
    all_tips_[new_dag_tip->id] = new_dag_tip;
  }
}

//
// TODO(HUT): make sure this is solid
// Add a DAG node
void DAG::AdvanceTipsInternal(DAGNodePtr node)
{
  // At this point, the node was not already in the node pool thus it must be a tip
  DAGTipPtr new_dag_tip =
      std::make_shared<DAGTip>(node->hash, node->oldest_epoch_referenced, node->weight);

  // Possibility that one or more prev references 'steals' tip status
  for (auto const &dag_node_prev : node->previous)
  {
    // Here we find a vector of dag tips that pointed at the hash
    // TODO(HUT): in future, keep track of extended tip info like seen TXs
    auto it = tips_.find(dag_node_prev);
    if (it != tips_.end())
    {
      DeleteTip(dag_node_prev);
    }
  }

  tips_[node->hash]          = new_dag_tip;
  all_tips_[new_dag_tip->id] = new_dag_tip;
}

// Check whether a node is invalid (non circular etc)
// TODO(HUT): this
bool DAG::NodeInvalidInternal(DAGNodePtr node)
{
  if (node->previous.size() == 0)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Invalid dag node seen : no prev references");
    return true;
  }

  return false;
}

// TODO(HUT): this.
bool DAG::SatisfyEpoch(DAGEpoch const &epoch)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Satisfying epoch: ", epoch.block_number);
  FETCH_LOCK(mutex_);

  if (epoch.block_number == 0)
  {
    return true;
  }

  if (epoch.block_number != most_recent_epoch_ + 1)
  {
    FETCH_LOG_WARN(
        LOGGING_NAME,
        "Attempt to satisfy a bad epoch: not an increment on the current DAG epoch! Current: ",
        most_recent_epoch_, " Satisfy: ", epoch.block_number);
    return false;
  }

  auto IsInvalid = [this](DAGNodePtr node) {
    if (node->previous.size() == 0)
    {
      return true;
    }

    // Node points to an epoch
    if (node->previous.size() == 1)
    {
      auto     node_prev_hash = *node->previous.begin();
      DAGEpoch points_to;
      bool     found = false;

      if (node_prev_hash == previous_epoch_.hash)
      {
        points_to = previous_epoch_;
        found     = true;
      }

      for (auto const &epoch : previous_epochs_)
      {
        if (node_prev_hash == epoch.hash)
        {
          found     = true;
          points_to = epoch;
          break;
        }
      }

      if (!found)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "DAG node found that points to unknown epoch");
        return true;
      }

      if (node->oldest_epoch_referenced != points_to.block_number)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "DAG node found with incorrect oldest_epoch_referenced :",
                       node->oldest_epoch_referenced, " while epoch is: ", points_to.block_number);
        return true;
      }

      if (node->weight != 0)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "DAG node found with incorrect weight");
        return true;
      }
    }
    else  // normal case, dag node references two others
    {
      uint64_t previous_hashes_oldest_allowed =
          previous_epochs_.empty() ? 0 : previous_epochs_.front().block_number;
      uint64_t   previous_hashes_oldest   = std::numeric_limits<uint64_t>::max();
      uint64_t   previous_hashes_heaviest = 0;
      DAGNodePtr getme;

      for (auto const &prev_hash : node->previous)
      {
        bool dummy;
        getme = GetDAGNodeInternal(prev_hash, true, dummy);
        if (!getme)
        {
          return true;
        }
        previous_hashes_oldest   = std::min(previous_hashes_oldest, getme->oldest_epoch_referenced);
        previous_hashes_heaviest = std::max(previous_hashes_heaviest, getme->weight);
      }

      if (node->oldest_epoch_referenced < previous_hashes_oldest_allowed)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Malformed node found in epoch: refers to too old a node");
        return true;
      }

      if (node->oldest_epoch_referenced != previous_hashes_oldest)
      {
        FETCH_LOG_WARN(LOGGING_NAME, " condition one has happened");
      }

      if (node->oldest_epoch_referenced != previous_hashes_oldest ||
          node->weight != previous_hashes_heaviest + 1)
      {
        FETCH_LOG_WARN(LOGGING_NAME,
                       "Malformed node found in epoch: doesn't increment weight or refer to oldest "
                       "correctly. Oldest: ",
                       node->oldest_epoch_referenced, "  weight: ", node->weight);
        return true;
      }
    }

    return false;
  };

  DAGNodePtr dag_node_to_add;
  bool       success       = true;
  uint64_t   missing_count = 0;
  uint64_t   loose_count   = 0;

  for (auto const &node_hash : epoch.all_nodes)
  {
    bool was_loose = false;
    // Note: this includes loose blocks, but it will be checked later that there are no loose blocks
    // in the final epoch
    auto dag_node_to_add = GetDAGNodeInternal(node_hash, true, was_loose);

    if (was_loose)
    {
      loose_count++;
    }

    if (!dag_node_to_add)
    {
      success = false;
      if (missing_count < 10)
      {
        FETCH_LOG_INFO(LOGGING_NAME,
                       "Found missing DAG node/hash when satisfying epoch: ", node_hash.ToBase64());
      }
      missing_.insert(node_hash);
      missing_count++;
      continue;
    }

    if (IsInvalid(dag_node_to_add))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Invalid dag node found in epoch!");
      return false;
    }
  }

  if (missing_count || loose_count)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "When satisfying, epoch ", epoch.block_number,
                   " AKA: ", epoch.hash.ToBase64(), " is missing : ", missing_count, " of ",
                   epoch.all_nodes.size(), ". Loose: ", loose_count);
  }

  // TODO(HUT): Verify Epoch here (loose nodes)

  return success;
}

// TODO(HUT): this
bool DAG::RevertToEpoch(uint64_t epoch_bn_to_revert)
{
  FETCH_LOCK(mutex_);

  if (epoch_bn_to_revert == 0)
  {
    epochs_.New(db_name_ + "_epochs.db", db_name_ + "_epochs.index.db");

    most_recent_epoch_ = 0;
    previous_epochs_.clear();
    all_tips_.clear();
    tips_.clear();
    node_pool_.clear();
    loose_nodes_.clear();
    loose_nodes_lookup_.clear();
    recently_added_.clear();
    missing_.clear();

    previous_epoch_ = DAGEpoch{};
    previous_epoch_.Finalise();
    return true;
  }

  if (epoch_bn_to_revert == most_recent_epoch_)
  {
    return true;
  }

  if (epoch_bn_to_revert > most_recent_epoch_)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Attempting to restore to epoch forward in time! Requested: ",
                   epoch_bn_to_revert, " while dag is at: ", most_recent_epoch_,
                   ". This is invalid.");
    return false;
  }

  auto GetEpochAndErase = [this](uint64_t current_index, bool erase) -> DAGEpoch {
    if (current_index == most_recent_epoch_)
    {
      return previous_epoch_;
    }

    for (auto const &epoch : previous_epochs_)
    {
      if (epoch.block_number == current_index)
      {
        return epoch;
      }
    }

    EpochHash getme;
    epochs_.Get(storage::ResourceAddress(std::to_string(current_index)), getme);

    if (erase)
    {
      epochs_.Erase(storage::ResourceAddress(std::to_string(current_index)));
    }

    DAGEpoch ret;
    all_stored_epochs_.Get(storage::ResourceID(getme), ret);
    return ret;
  };

  std::vector<DAGNodePtr> nodes_to_readd;
  uint64_t                current_index = most_recent_epoch_;

  while (current_index != epoch_bn_to_revert)
  {
    DAGEpoch epoch_to_erase = GetEpochAndErase(current_index, true);

    for (auto const &dag_node_hash : epoch_to_erase.all_nodes)
    {
      bool dummy;
      nodes_to_readd.push_back(GetDAGNodeInternal(dag_node_hash, false, dummy));
    }
    current_index--;
  }

  // Set HEAD and previous epochs
  current_index = epoch_bn_to_revert;

  // TODO(HUT): could be more efficient
  std::deque<DAGEpoch> replacement_epochs;

  while (current_index != 0 && (replacement_epochs.size() < (EPOCH_VALIDITY_PERIOD)))
  {
    DAGEpoch epoch_to_set = GetEpochAndErase(current_index, false);
    replacement_epochs.push_front(epoch_to_set);
    current_index--;
  }

  previous_epochs_ = std::move(replacement_epochs);
  previous_epoch_  = previous_epochs_.back();
  previous_epochs_.pop_back();
  most_recent_epoch_ = epoch_bn_to_revert;

  assert(previous_epochs_.size() < EPOCH_VALIDITY_PERIOD);

  // In the case that there has been a revert these nodes are not deleted from storage (for now -
  // need to think about it) because it might be immediately used on start up. They are not readded
  // to the pool though.
  const bool add_nodes_to_pool = false;

  if (add_nodes_to_pool)
  {
    for (auto const &node : nodes_to_readd)
    {
      PushInternal(node);
    }
  }

  return true;
}

bool DAG::GetEpochFromStorage(std::string const &identifier, DAGEpoch &epoch)
{
  EpochHash getme;

  // Need to get hash of epoch first (default key is block number as string)
  if (!epochs_.Get(storage::ResourceAddress(identifier), getme))
  {
    return false;
  }

  return all_stored_epochs_.Get(storage::ResourceID(getme), epoch);
}

bool DAG::SetEpochInStorage(std::string const &, DAGEpoch const &epoch, bool is_head)
{
  all_stored_epochs_.Set(storage::ResourceID(epoch.hash), epoch);  // Store of all epochs
  epochs_.Set(storage::ResourceAddress(std::to_string(epoch.block_number)),
              epoch.hash);  // Our epoch stack

  if (is_head)
  {
    epochs_.Set(storage::ResourceAddress("HEAD"), epoch.hash);
  }

  return true;
}

uint64_t DAG::CurrentEpoch() const
{
  FETCH_LOCK(mutex_);
  return most_recent_epoch_;
}

bool DAG::HasEpoch(EpochHash const &hash)
{
  FETCH_LOCK(mutex_);

  // Check for current before checking the file
  if (hash == previous_epoch_.hash)
  {
    return true;
  }

  for (auto const &epoch : previous_epochs_)
  {
    if (hash == epoch.hash)
    {
      return true;
    }
  }

  DAGEpoch dummy;
  return all_stored_epochs_.Get(storage::ResourceID(hash), dummy);
}

// Delete tip by id
void DAG::DeleteTip(DAGTipID tip_id)
{
  auto tip = all_tips_.at(tip_id);

  tips_.erase(tip->dag_node_reference);
  all_tips_.erase(tip_id);
}

// Delete tip by hash
void DAG::DeleteTip(NodeHash hash)
{
  auto tip = tips_.at(hash);

  all_tips_.erase(tip->id);
  tips_.erase(hash);
}
