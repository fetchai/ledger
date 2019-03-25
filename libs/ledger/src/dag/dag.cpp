//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "ledger/dag/dag.hpp"

namespace fetch {
namespace ledger {

DAG::DAG()
{
  LOG_STACK_TRACE_POINT;
  // Creating the initial genesis node.
  // It is important that the initial DAG node is not a block
  // as the DAG nodes are not allowed to refer to blocks.
  DAGNode n;
  n.type = DAGNode::WORK;
  n.contents = "genesis"; // TODO(tfr): make configurable

  // Use PushInternal to bypass checks on previous hashes
  PushInternal(n);
}

/**
 * @brief push a node to the DAG.
 *
 * @param node is the node to be added.
 */
bool DAG::Push(DAGNode node)
{
  FETCH_LOCK(maintenance_mutex_);  

  LOG_STACK_TRACE_POINT;

  // Checking that the previous nodes are valid.
  if(!ValidatePreviousInternal(node))
  {
    FETCH_LOG_ERROR("DAG", "Could not validate previous");
    return false;
  }

  return PushInternal(std::move(node));
}


/**
 * @brief returns a vector copy of the DAG.
 */
DAG::NodeArray DAG::DAGNodes() const
{
  FETCH_LOCK(maintenance_mutex_);
  LOG_STACK_TRACE_POINT;
  NodeArray nodes;
  nodes.reserve(nodes_.size());

  for (auto const &pair: nodes_)
  {
    nodes.push_back(pair.second);
  }

  return nodes;
}

/**
 * @brief number of blocks available.
 */
uint64_t DAG::node_count()
{
  LOG_STACK_TRACE_POINT;  
  FETCH_LOCK(maintenance_mutex_);  
  return static_cast<uint64_t>(all_node_hashes_.size());
}

/**
 * @brief number of blocks available.
 */
DAG::NodeArray DAG::GetChunk(uint64_t const &f, uint64_t const &t)
{
  LOG_STACK_TRACE_POINT;
  FETCH_LOCK(maintenance_mutex_);  

  assert(t >= f);

  NodeArray ret;
  ret.reserve(t - f);

  for (uint64_t i = f; i < t; ++i)
  {
    auto hash = all_node_hashes_.at(i);

    if(nodes_.find(hash) == nodes_.end())
    {
      FETCH_LOG_ERROR("DAG", "Could not find hash ",byte_array::ToBase64(hash));
      continue;
    }
    
    ret.push_back( nodes_[hash] );
  }

  return ret;
}


bool DAG::ValidatePreviousInternal(DAGNode const &node)
{
  LOG_STACK_TRACE_POINT;
  // The node is only valid if it refers existing nodes.
  bool is_valid = (node.previous.size() > 0);
  for(auto &h: node.previous)
  {
    // and if those nodes exists.
    auto it = nodes_.find(h);
    if(it == nodes_.end())
    {
      FETCH_LOG_INFO("DAG", "could not find preivous.");
      is_valid = false;
      break;
    }

  }

  if(!is_valid)
  {
    // TODO: Not valid
    return false;
  }
  return true;
}

/**
 * @brief internal pushing mechanism.
 *
 * @param node is the node to be added to the DAG.
 */
bool DAG::PushInternal(DAGNode node)
{
  LOG_STACK_TRACE_POINT;
  // Finalise and get the node time.
  node.Finalise();

  // CLearing all tips that are being referenced a lot
  for(auto &h: node.previous)
  {
    auto it = tips_.find(h);
    if(it != tips_.end())
    {
      it->second += 1;
      if(it->second > PARAMETER_REFERENCES_TO_BE_TIP)
      {
        tips_.erase(it);
      }
    }
  }

  // Adding node
  tips_.insert({node.hash, 0});

  nodes_[node.hash] = node;
  all_node_hashes_.push_back(node.hash);

  SignalNewNode(node);
  return true;
}


DAG::DigestVector DAG::UncertifiedTipsAsVector() const
{
  FETCH_LOCK(maintenance_mutex_);
  DigestVector ret{};
  for(auto const &t: tips_)
  {
    // TODO(tfr): Check whether certified
    ret.push_back(t.first);
  }
  return ret;
}

} // namespace ledger
} // namespace fetch
