#pragma once

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/sha256.hpp"
#include "crypto/fnv.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/mutex.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/identity.hpp"
#include "ledger/dag/dag_node.hpp"


#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <list>
#include <limits>
#include <queue>
#include <deque>

namespace fetch {
namespace ledger {

/**
 * DAG implementation.
 */
class DAG
{
public:

  using ConstByteArray  = byte_array::ConstByteArray;
  using Digest          = ConstByteArray;
  using DigestCache     = std::deque<Digest>;
  using DigestArray     = std::vector<Digest>;
  using NodeList        = std::list<DAGNode>;
  using NodeArray       = std::vector<DAGNode>;
  using NodeMap         = std::unordered_map<Digest, DAGNode>;
  using DigestSet         = std::unordered_set<Digest>;
  using DigestVector      = std::vector<Digest>;  
  using Mutex             = mutex::Mutex;
  using CallbackFunction  = std::function< void(DAGNode) > ;
  
  // Construction / Destruction
  DAG();
  DAG(DAG const &) = delete;
  DAG(DAG &&) = delete;
  ~DAG() = default;

  bool Push(DAGNode node);
  bool PushBlock(DAGNode node);
  DigestVector UncertifiedTipsAsVector() const;
  DigestSet const tips() const;
  DigestSet const tips_unsafe() const;

  DigestCache const last_nodes() const;
  NodeMap const nodes() const;
  NodeList const block_nodes() const;

  bool HasNode(Digest const &hash);

  NodeArray DAGNodes() const;

  uint64_t node_count();

  NodeArray GetChunk(uint64_t const &f, uint64_t const &t);

  // Operators
  DAG &operator=(DAG const &) = delete;
  DAG &operator=(DAG &&) = delete;

  void OnNewNode(CallbackFunction cb)
  {
    LOG_STACK_TRACE_POINT;      
    on_new_node_ = std::move(cb);
  }


  /// Control logic for setting the time of the nodes.
  /// @{
  bool SetNodeTime(uint64_t block_number, DigestArray hashes)
  {
    FETCH_LOCK(maintenance_mutex_); 

    std::vector< Digest > updated_hashes{};
    std::queue< Digest >  queue;

    // Preparing graph traversal
    for(auto const& h :hashes)
    {
      queue.push(h);
    }

    // Traversing
    while(!queue.empty())
    {
      auto hash = queue.front();
      queue.pop();
    
      if(nodes_.find(hash) == nodes_.end())
      {
        // Revert to previous state if an invalid hash is given
        for(auto &h: updated_hashes)
        {
          auto &node = nodes_[h];
          h.timestamp = DAGNode::INVALID_TIMESTAMP;
        }
        // FETCH_LOG_ERROR
        return false;
      }

      // Checking if the node was already certified.
      auto &node = nodes_[hash];
      if(node.timestamp == DAGNode::INVALID_TIMESTAMP)
      {

        // If not we certify it and proceed to its parents
        node.timestamp = block_number;
        for(auto &p: node.previous)
        {
          queue.push(p);
        }

        // Keeping track of updated nodes in case we need to revert.
        updated_hashes.push_back(hash);
      }
    }

    return true;
  }

  void RevertTo(uint64_t block_number)
  {
    FETCH_LOCK(maintenance_mutex_);
    // TODO: Implement to revert.
  }

  NodeArray ExtractSegment(uint64_t block_number)
  {
    FETCH_LOCK(maintenance_mutex_);    
    // TODO: implement.
    return {};
  }
  /// }

 /**
 * @brief validates the previous nodes referenced by a node.
 *
 * @param node is the node which needs to be checked.
 */
  bool ValidatePrevious(DAGNode const &node)
  {
    FETCH_LOCK(maintenance_mutex_);
    return ValidatePreviousInternal(node);
  }

private:
  // Not thread safe
  bool ValidatePreviousInternal(DAGNode const &node);
  void SignalNewNode(DAGNode n)
  {
    LOG_STACK_TRACE_POINT;      
    if(on_new_node_) on_new_node_(std::move(n));
  }


  bool PushInternal(DAGNode node);

  DigestCache last_nodes_;                   ///< buffer used when creating new nodes to ensure enough referencing.
  NodeMap     nodes_;                        ///< the full DAG.
  DigestSet   tips_;                         ///< tips of the DAG.
  DigestArray all_node_hashes_;              ///< contain all node hashes
  CallbackFunction on_new_node_;  
  mutable Mutex     maintenance_mutex_{__LINE__, __FILE__};  
};

/**
 * @brief returns the tips of the DAG.
 */
inline std::unordered_set< byte_array::ConstByteArray > const DAG::tips() const
{
  FETCH_LOCK(maintenance_mutex_);    
  return tips_;
}

/**
 * @brief returns the tips of the DAG.
 */
inline std::unordered_set< byte_array::ConstByteArray > const DAG::tips_unsafe() const
{
  return tips_;
}


/**
 * @brief returns the last nodes.
 *
 * TODO(tfr): make the number of cached last nodes configurable.
 */
inline DAG::DigestCache const DAG::last_nodes() const
{
  FETCH_LOCK(maintenance_mutex_);  
  return last_nodes_;
}

/**
 * @brief returns all nodes.
 */
inline DAG::NodeMap const DAG::nodes() const
{
  FETCH_LOCK(maintenance_mutex_);  
  return nodes_;
}


/**
 * @brief checks whether a given hash is found in the DAG.
 */
inline bool DAG::HasNode(byte_array::ConstByteArray const &hash)
{
  FETCH_LOCK(maintenance_mutex_);    
  return nodes_.find(hash) != nodes_.end();
}

} // namespace ledger
} // namespace fetch
