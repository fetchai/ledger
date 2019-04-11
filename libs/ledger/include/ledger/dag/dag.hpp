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
#include "ledger/chain/block.hpp"
#include "ledger/dag/dag_interface.hpp"

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
class DAG : public DAGInterface
{
public:
  using VerifierType      = crypto::ECDSAVerifier;
  using ConstByteArray    = byte_array::ConstByteArray;
  using Digest            = ConstByteArray;
  using DigestCache       = std::deque<Digest>;
  using DigestArray       = std::vector<Digest>;
  using NodeList          = std::list<DAGNode>;
  using NodeArray         = std::vector<DAGNode>;
  using NodeDeque         = std::deque<DAGNode>;
  using NodeMap           = std::unordered_map<Digest, DAGNode>;
  using DigestMap         = std::unordered_map<Digest, uint64_t>;
  using Mutex             = mutex::Mutex;
  using CallbackFunction  = std::function< void(DAGNode) > ;
  
  static const uint64_t PARAMETER_REFERENCES_TO_BE_TIP = 2;

  // Construction / Destruction
  DAG(ConstByteArray genesis_contents = "genesis");
  DAG(DAG const &)            = delete;
  DAG(DAG &&)                 = delete;
  virtual ~DAG()              = default;

  // Operators
  DAG &operator=(DAG const &) = delete;
  DAG &operator=(DAG &&)      = delete;

  /// Methods to interact with the dag
  /// @{
  bool Push(DAGNode node) override;  
  bool HasNode(Digest const &hash) override;
  NodeArray DAGNodes() const override;  
  DigestArray UncertifiedTipsAsVector() const override; 
  DigestMap const tips() const override;
  NodeMap const nodes() const override;
  uint64_t node_count() override;
  void RemoveNode(Digest const & hash) override;
  /// }


  /// Methods used for syncing
  /// @{
  NodeArray GetChunk(uint64_t const &f, uint64_t const &t) override;
  NodeArray GetBefore(DigestArray hashes, uint64_t const &block_number, uint64_t const &count) override;
  NodeArray GetAfter(DigestArray hashes, uint64_t const &block_number, uint64_t const &count) override;
  NodeDeque GetLatest() const override;
  /// }

  /// Control logic for setting the time of the nodes.
  /// @{
  bool SetNodeTime(Block const&block) override;
  NodeArray ExtractSegment(Block const &block) override;
  void SetNodeReferences(DAGNode &node, int count = 2) override;

  void RevertTo(uint64_t /*block_number*/) override
  {
    FETCH_LOCK(maintenance_mutex_);
    // TODO: Implement to revert.
    throw std::runtime_error("DAG::RevertTo not implemented");    
  }
  /// }

  /**
  * @brief validates the previous nodes referenced by a node.
  *
  * @param node is the node which needs to be checked.
  */
  bool ValidatePrevious(DAGNode const &node) override
  {
    FETCH_LOCK(maintenance_mutex_);
    return ValidatePreviousInternal(node);
  }

  std::size_t size() const { return nodes_.size(); } // TODO: check whether can be deleted
private:
  bool ValidatePreviousInternal(DAGNode const &node);
  void SignalNewNode(DAGNode n)
  {
    LOG_STACK_TRACE_POINT;      
    if(on_new_node_) on_new_node_(std::move(n));
  }

  bool PushInternal(DAGNode node, bool check_signature = true);

  NodeMap           nodes_;                        ///< the full DAG.
  DigestMap         tips_;                         ///< tips of the DAG.
  DigestArray       all_node_hashes_;              ///< contain all node hashes
  CallbackFunction  on_new_node_;  
  NodeDeque         latest_;

  mutable Mutex     maintenance_mutex_{__LINE__, __FILE__};
};

/**
 * @brief returns the tips of the DAG.
 */
inline DAG::DigestMap const DAG::tips() const
{
  FETCH_LOCK(maintenance_mutex_);    
  return tips_;
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
