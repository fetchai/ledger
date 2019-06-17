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

#include "ledger/dag/dag_epoch.hpp"
#include "ledger/dag/dag_node.hpp"
#include "ledger/upow/work.hpp"
#include "network/p2pservice/p2p_service.hpp"

namespace fetch {
namespace ledger {

/**
 * DAG implementation.
 */
class DAGInterface
{
public:
  using ConstByteArray = byte_array::ConstByteArray;
  using NodeHash       = ConstByteArray;
  using EpochHash      = ConstByteArray;
  using CertificatePtr = p2p::P2PService::CertificatePtr;

  // TODO(HUT): cleanly define things here
  using MissingTXs   = std::set<NodeHash>;
  using MissingNodes = std::set<DAGNode>;

  enum class DAGTypes
  {
    DATA
  };

  DAGInterface()          = default;
  virtual ~DAGInterface() = default;

  // TODO(HUT): resolve this part of the interface
  virtual void AddTransaction(Transaction const &tx, DAGTypes type) = 0;
  virtual void AddWork(Work const &work)                            = 0;
  virtual void AddArbitrary(ConstByteArray const &payload)          = 0;

  virtual DAGEpoch             CreateEpoch(uint64_t block_number)          = 0;
  virtual bool                 CommitEpoch(DAGEpoch)                       = 0;
  virtual bool                 RevertToEpoch(uint64_t)                     = 0;
  virtual uint64_t             CurrentEpoch() const                        = 0;
  virtual bool                 HasEpoch(EpochHash const &hash)             = 0;
  virtual bool                 SatisfyEpoch(DAGEpoch const &)              = 0;
  virtual std::vector<DAGNode> GetLatest(bool previous_epoch_only = false) = 0;

  // Functions used for syncing
  virtual std::vector<DAGNode> GetRecentlyAdded()                                    = 0;
  virtual MissingTXs           GetRecentlyMissing()                                  = 0;
  virtual bool                 GetDAGNode(ConstByteArray const &hash, DAGNode &node) = 0;
  virtual bool                 GetWork(ConstByteArray const &hash, Work &work)       = 0;
  virtual bool                 AddDAGNode(DAGNode node)                              = 0;
};

}  // namespace ledger
}  // namespace fetch
