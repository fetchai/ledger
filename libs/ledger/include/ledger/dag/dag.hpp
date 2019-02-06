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
namespace dag {

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
  using Mutex             = mutex::Mutex;
  using CallbackFunction  = std::function< void(DAGNode) > ;
  
  // Construction / Destruction
  DAG();
  DAG(DAG const &) = delete;
  DAG(DAG &&) = delete;
  ~DAG() = default;

  uint64_t GetNodeTime(DAGNode const & node);

  bool Push(DAGNode node);
  bool PushBlock(DAGNode node);
  DigestSet const tips() const;

  DigestCache const last_nodes() const;
  NodeMap const nodes() const;
  NodeList const block_nodes() const;
  uint64_t time_since_last_block() const;
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

  bool ValidatePrevious(DAGNode const &node);
private:
  void SignalNewNode(DAGNode n)
  {
    LOG_STACK_TRACE_POINT;      
    if(on_new_node_) on_new_node_(std::move(n));
  }


  bool PushInternal(DAGNode node);

  uint64_t    time_since_last_block_ = 0;    ///< keeps track of the time since last block.
  NodeList    block_nodes_;
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
 * @brief returns block nodes.
 */
inline DAG::NodeList const DAG::block_nodes() const
{
  FETCH_LOCK(maintenance_mutex_);  
  return block_nodes_;
}

/**
 * @brief returns the time that has passed since last block
 */
inline uint64_t DAG::time_since_last_block() const
{
  FETCH_LOCK(maintenance_mutex_);    
  return time_since_last_block_;
}

/**
 * @brief checks whether a given hash is found in the DAG.
 */
inline bool DAG::HasNode(byte_array::ConstByteArray const &hash)
{
  FETCH_LOCK(maintenance_mutex_);    
  return nodes_.find(hash) != nodes_.end();
}

} // namespace dag
} // namespace ledger
} // namespace fetch
