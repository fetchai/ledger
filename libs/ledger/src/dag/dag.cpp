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

DAG::DAG(ConstByteArray genesis_contents)
{
  LOG_STACK_TRACE_POINT;
  // Creating the initial genesis node.
  // It is important that the initial DAG node is not a block
  // as the DAG nodes are not allowed to refer to blocks.
  DAGNode n;
  n.type = DAGNode::GENESIS;
  n.contents = std::move(genesis_contents);
  n.timestamp = DAGNode::GENESIS_TIME;

  // Use PushInternal to bypass checks on previous hashes
  PushInternal(n, false);
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


DAG::NodeArray DAG::GetBefore(DigestArray hashes, uint64_t const &block_number, uint64_t const &count) 
{
  FETCH_LOCK(maintenance_mutex_);
  NodeArray ret;

  std::queue< Digest > queue;
  std::unordered_set< Digest > added_to_queue;

  for(auto &t: hashes)
  {
    if(added_to_queue.find(t) != added_to_queue.end())
    {
      continue;
    }        
    added_to_queue.insert(t);
    queue.push(t);
  }

  while(!queue.empty())
  {
    auto &node = nodes_[queue.front()];
    queue.pop();

    // Ignoring genesis
    if(node.type == DAGNode::GENESIS)
    {

      continue;
    }

    // We stop once the time is below the 
    // desired block time.
    if(node.timestamp < block_number)
    {
      continue;
    }

    ret.push_back(node);
    if(ret.size() > count)
    {
      return ret;
    }
    
    for(auto &p: node.previous)
    {
      if(added_to_queue.find(p) != added_to_queue.end())
      {
        continue;
      }

      added_to_queue.insert( p );          
      queue.push(p);
    }
  }

  return ret;
}


DAG::NodeArray DAG::GetAfter(DigestArray hashes, uint64_t const &block_number, uint64_t const &count)
{
  FETCH_LOCK(maintenance_mutex_);
  NodeArray ret;

  std::queue< Digest > queue;
  std::unordered_set< Digest > added_to_queue;

  for(auto &t: hashes)
  {
    if(added_to_queue.find(t) != added_to_queue.end())
    {
      continue;
    }    
    added_to_queue.insert(t);
    queue.push(t);
  }

  while(!queue.empty())
  {
    auto &node = nodes_[queue.front()];
    queue.pop();

    // Ignoring genesis
    if(node.type == DAGNode::GENESIS)
    {
      continue;
    }

    // Stopping once the time passes the block time
    if(node.timestamp >= block_number)
    {
      continue;
    }

    ret.push_back(node);
    if(ret.size() > count)
    {
      return ret;
    }

    for(auto &p: node.next)
    {
      if(added_to_queue.find(p) != added_to_queue.end())
      {
        continue;
      }

      added_to_queue.insert( p );          
      queue.push(p);
    }
    
  }

  return ret;
}

DAG::NodeDeque DAG::GetLatest() const
{
  return latest_;
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
    return false;
  }
  return true;
}

bool DAG::SetNodeTime(Block const& block)
{
  FETCH_LOCK(maintenance_mutex_); 

  std::unordered_set< Digest > added_to_queue;
  std::vector< Digest > updated_hashes{};
  std::queue< Digest >  queue;

  // Preparing graph traversal
  for(auto const& h : block.body.dag_nodes)
  {
    added_to_queue.insert(h);
    queue.push(h);
  }

  // Traversing
  std::size_t n = 0;
  while(!queue.empty())
  {
    auto hash = queue.front();
    queue.pop();
  
    if(nodes_.find(hash) == nodes_.end())
    {
      // Revert to previous state if an invalid hash is given
      n = 0;
      for(auto &h: updated_hashes)
      {
        auto &node = nodes_[h];
        node.timestamp = DAGNode::INVALID_TIMESTAMP;
      }

      return false;
    }

    // Checking if the node was already certified.
    auto &node = nodes_[hash];
    if(node.timestamp == DAGNode::INVALID_TIMESTAMP)
    {

      // If not we certify it and proceed to its parents
      node.timestamp = block.body.block_number;
      for(auto &p: node.previous)
      {
        if(added_to_queue.find(p) != added_to_queue.end())
        {
          continue;
        }

        added_to_queue.insert( p );   
        queue.push(p);
      }

      // Keeping track of updated nodes in case we need to revert.
      updated_hashes.push_back(hash);
      ++n;
    }
  }

  return true;
}

void DAG::SetNodeReferences(DAGNode &node, int count)
{
  FETCH_LOCK(maintenance_mutex_);
  DigestArray tips_to_select_from;
  for(auto n: tips_)
  {
    tips_to_select_from.push_back(n.first);
  }    
  std::random_shuffle(tips_to_select_from.begin(), tips_to_select_from.end());

  while(count > 0)
  {
    node.previous.push_back(tips_to_select_from.back());
    if(tips_to_select_from.size() > 1)
    {
      tips_to_select_from.pop_back();
    }
    --count;
  }
}

DAG::NodeArray DAG::ExtractSegment(Block const &block)
{
  // It is important that the block is used to extract the segment 
  // and not just the block time. If the hashes starting the extract
  // are not consistent accross nodes, the order of the extract will
  // come out differently.    
  FETCH_LOCK(maintenance_mutex_);
  NodeArray ret;

  std::queue< Digest > queue;
  std::unordered_set< Digest > added_to_queue;

  for(auto &t: block.body.dag_nodes)
  {
    if(added_to_queue.find(t) != added_to_queue.end())
    {
      continue;
    }

    added_to_queue.insert(t);
    queue.push(t);
  }

  while(!queue.empty())
  {
    auto &node = nodes_[queue.front()];
    queue.pop();

    if((node.timestamp < block.body.block_number) && (node.timestamp != DAGNode::INVALID_TIMESTAMP))
    {
      continue;
    }

    if(node.timestamp == block.body.block_number)
    {
      ret.push_back(node);
    }

    for(auto &p: node.previous)
    {
      if(added_to_queue.find(p) != added_to_queue.end())
      {
        continue;
      }

      added_to_queue.insert( p );          
      queue.push(p);
    }
    
  }

  return ret;
}

void DAG::RemoveNode(Digest const & hash)
{
  FETCH_LOCK(maintenance_mutex_);

  std::queue< Digest > queue;
  std::unordered_set< Digest > added_to_queue;

  queue.push(hash);
  added_to_queue.insert(hash);

  while(!queue.empty())
  {
    auto h = queue.front();
    queue.pop();
    auto it = nodes_.find(h);

    for(auto &next: it->second.next)
    {
      if(added_to_queue.find(next) == added_to_queue.end())
      {
        queue.push(next);
        added_to_queue.insert(next);
      }
    }

    nodes_.erase(it);
  }


}

/**
 * @brief internal pushing mechanism.
 *
 * @param node is the node to be added to the DAG.
 */
bool DAG::PushInternal(DAGNode node, bool check_signature)
{
  LOG_STACK_TRACE_POINT;

  // Finalise to get the node hash and force no time on them
  node.Finalise();
  node.timestamp = DAGNode::INVALID_TIMESTAMP;

  // Special treatment of genesis node
  if(nodes_.size() == 0)
  {
    node.timestamp = DAGNode::GENESIS_TIME;
  }

  if(check_signature)
  {
    // Ensuring that the identity is not empty
    if(!static_cast<bool>(node.identity))
    {
      return false;
    }

    // Checking the signature
    VerifierType verfifier(node.identity);
    if (!verfifier.Verify(node.hash, node.signature))
    {
      // TODO: Update trust system
      return false;
    }
  }

  for(auto &h: node.previous)
  {
    // CLearing all tips that are being referenced a lot    
    auto it = tips_.find(h);
    if(it != tips_.end())
    {
      it->second += 1;
      if(it->second > PARAMETER_REFERENCES_TO_BE_TIP)
      {
        tips_.erase(it);
      }
    }

    // Updating all referenced nodes    
    auto &prev = nodes_[h];
    prev.next.insert(node.hash);
  }

  // Adding node
  tips_.insert({node.hash, 0});

  nodes_[node.hash] = node;
  all_node_hashes_.push_back(node.hash);

  // Updating sync cache
  latest_.push_back(node);
  while(latest_.size() > LATEST_CACHE_SIZE)
  {
    latest_.pop_front();
  }

  SignalNewNode(node);
  return true;
}


DAG::DigestArray DAG::UncertifiedTipsAsVector() const
{
  FETCH_LOCK(maintenance_mutex_);
  DigestArray ret{};

  for(auto const &t: tips_)
  {
    auto it = nodes_.find(t.first);
    if(it->second.timestamp != DAGNode::INVALID_TIMESTAMP)
    {
      continue;
    }
    ret.push_back(t.first);
  }
  return ret;
}

} // namespace ledger
} // namespace fetch
