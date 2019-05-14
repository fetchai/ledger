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
#include "core/byte_array/const_byte_array.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/dag/dag_node.hpp"

#include <deque>
#include <map>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace ledger {

/**
 * DAG implementation.
 */
class DAGInterface
{
public:
  using ConstByteArray = byte_array::ConstByteArray;
  using Digest         = ConstByteArray;
  using DigestArray    = std::vector<Digest>;
  using NodeArray      = std::vector<DAGNode>;
  using NodeDeque      = std::deque<DAGNode>;
  using NodeMap        = std::unordered_map<Digest, DAGNode>;
  using DigestMap      = std::unordered_map<Digest, uint64_t>;

  DAGInterface &operator=(DAGInterface const &) = delete;
  DAGInterface &operator=(DAGInterface &&) = delete;

  virtual bool            Push(DAGNode node)              = 0;
  virtual bool            HasNode(Digest const &hash)     = 0;
  virtual NodeArray       DAGNodes() const                = 0;
  virtual DigestArray     UncertifiedTipsAsVector() const = 0;
  virtual DigestMap const tips() const                    = 0;
  virtual NodeMap const   nodes() const                   = 0;
  virtual uint64_t        node_count()                    = 0;
  virtual void            RemoveNode(Digest const &hash)  = 0;

  virtual NodeArray GetChunk(uint64_t const &f, uint64_t const &t)             = 0;
  virtual NodeArray GetBefore(DigestArray, uint64_t const &, uint64_t const &) = 0;
  virtual NodeArray GetAfter(DigestArray, uint64_t const &, uint64_t const &)  = 0;
  virtual NodeDeque GetLatest() const                                          = 0;

  virtual bool      SetNodeTime(Block const &block)                 = 0;
  virtual NodeArray ExtractSegment(Block const &block)              = 0;
  virtual void      SetNodeReferences(DAGNode &node, int count = 2) = 0;

  virtual void RevertTo(uint64_t /*block_number*/)   = 0;
  virtual bool ValidatePrevious(DAGNode const &node) = 0;
};

}  // namespace ledger
}  // namespace fetch
