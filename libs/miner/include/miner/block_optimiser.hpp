#ifndef MINER_BLOCK_OPTIMISER_HPP
#define MINER_BLOCK_OPTIMISER_HPP
#include "core/assert.hpp"

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/fnv.hpp"
#include "math/rectangular_array.hpp"

#include "ledger/chain/block.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"
#include "ledger/chain/transaction.hpp"

#include "miner/optimisation/binary_annealer.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace fetch {
namespace chain {

class BlockGenerator {
public:
  typedef std::shared_ptr<TransactionSummary> shared_transaction_type;
  typedef crypto::CallableFNV hasher_type;
  
  // Block defs
  typedef fetch::chain::consensus::ProofOfWork proof_type;
  typedef fetch::chain::BlockBody block_body_type;
  typedef typename proof_type::header_type block_header_type;
  typedef fetch::chain::BasicBlock<proof_type, fetch::crypto::SHA256>
  block_type;
  typedef std::shared_ptr<block_type> shared_block_type;
  typedef fetch::optimisers::BinaryAnnealer annealer_type;
  typedef typename annealer_type::state_type state_type;



  /* Pushes a transaction into the queue of transactions that needs to
   * be mined.
   * @tx is the transaction summary of the transaction.
   * @check whether to check if the transaction is already known.
   *
   * If check is false, the transaction is registered and marked as
   * unspent. It will only appear once in the register with all
   * transactions. 
   */
  void PushTransactionSummary(TransactionSummary const &tx, bool check = true) {
    shared_transaction_type stx = std::make_shared<TransactionSummary>(tx);

    if(check) { 
      if (all_.find(stx->transaction_hash) != all_.end()) return;
    }
    
    all_[stx->transaction_hash] = stx;

    unspent_.push_back(stx);
  }
  

  /* Configures the annealing algorithm used for the heuristic search.
   * @sweeps is the number of sweeps done by the annealer.
   * @b0 is the initial inverse temperature.
   * @b1 is the final inverse temperature.
   *
   * We define a sweep as one attempted variable update for every binary
   * variable in the problem considered. Normally the optimal amount of
   * sweeps depends on the problem size (i.e. the number of lanes and
   * the batch size). Likewise the inverse temperatures are likely to
   * change as the problem is changing.
   */
  void ConfigureAnnealer(std::size_t const &sweeps, double const& b0, double const& b1 ) {
    annealer_.SetSweeps(sweeps);
    annealer_.SetBetaStart(b0);
    annealer_.SetBetaEnd(b1);
  }

  /* Generates the next block.
   * @lane_count is the number of count.
   * @slice_count is the number of slices.
   * @strategy defines the strategy for transaction inclusion.
   * @batch_size the number of transactions considered for each slice.
   * @explore it the number of solutions considered for each slice.
   *
   * This method populates a block with a number of block slices. The
   * block slices are generated one at a time using a variant of
   * simulated annealing for binary optimsations problems. The algorithm
   * heuristically searches for optimal block slices (although most of
   * the time sub optimal ones will be found) and populates the block
   * slice by slice.
   *
   * The resulting block is garantueed (TODO) to be valid.
   */
  void GenerateBlock(std::size_t const &lane_count,
    std::size_t const &slice_count, int const &strategy = 0,
    std::size_t batch_size = 1, std::size_t explore = 10) 
  {
    block_.clear();
    block_fees_.clear();
    occupancy_ = 0;
    
    for(std::size_t j=0; j < slice_count; ++j) {
      Init(lane_count, strategy, batch_size);
      
      // Computing next slice      
      for(std::size_t i=0; i < explore; ++i) {
        GenerateBlockSlice();
      }
      
      // Adding next slice to block
      block_.push_back( std::vector< uint64_t >() );
      auto &vec = block_.back();
      
      std::vector< std::size_t > used;

      // TODO: Correct incorrect states
      std::size_t i = 0;
      for(auto &s: best_solution_) {
        if(s == 1) {
          auto &tx = unspent_[i];
          occupancy_ += tx->groups.size();
          vec.push_back(tx->short_id);
          used.push_back(i);
        }
        ++i;        
      }
      
      block_fees_.push_back( best_solution_energy_ );

      // Erasing from unspent pool
      std::reverse(used.begin(), used.end());
      for(auto &i: used) {
        staged_.push_back(unspent_[i]);        
        unspent_[i] = unspent_[ unspent_.size() - 1 ];
        unspent_.pop_back();
      }

    }
    
  }
  

  /* Unstages staged transactions.
   * 
   * This method is can be used to reset the state of the unspent
   * transactions after generating a block.
   */
  void Reset() 
  {
    for(auto &tx: staged_) {
      unspent_.push_back(tx);
    }
    staged_.clear();
  }
  

  /* Returns a constant reference to the vector containing unspent
   * transactions.
   */
  std::vector<shared_transaction_type> const &unspent() const {
    return unspent_;
  };


  /* Returns the number of unspent transactions.
   */
  std::size_t unspent_count() const 
  {
    return unspent_.size();
  }

  /* Returns the lane count.
   */
  std::size_t lane_count() { return lane_count_; }

  /* Returns the batch size.
   */
  std::size_t batch_size() { return batch_size_; }

  /* Returns a constant reference to the last generated block.
   *
   * TODO: change to system block
   */
  std::vector< std::vector< uint64_t > > const & block() const { return block_; }

  /* Returns a constant reference to a vector with the fees earned from
   * each slice.
   */  
  std::vector< double > const & block_fees() const { return block_fees_; }


  /* Returns the absolute number of 
   */    
  double  const & block_occupancy() { return occupancy_; }

  /*
  std::vector< int16_t > const & block() const {
    return block_;
  }
  */
 private:

  /* Prepares the class instance to generate the next block slice.
   * @lane_count is the number of lanes.
   * @strategy is the strategy by which a batch is generated.
   * @batch_size is the size of the batch used to generate a solution.
   * @penalty is the penalty used to prevent conflicting transactions.
   *
   * This function identifies conflicts between transactions and uses
   * this to create the binary optimisation problem.
   */
  void Init(std::size_t const &lane_count = 16,
    int strategy = 0,    
    std::size_t batch_size = std::size_t(- 1),
    uint64_t penalty = 10)
  {

    best_solution_energy_ = 0;
    state_energy_ = 0;  
    best_solution_.clear();
    state_.clear();
    
    switch(strategy) {
    case 1:
      std::sort( unspent_.begin(), unspent_.end(), [](std::shared_ptr<TransactionSummary> const &a, std::shared_ptr<TransactionSummary> const &b) {
          return (a->fee / a->groups.size())  > (b->fee / b->groups.size());
        });
      break;
    case 2:
      
      std::sort( unspent_.begin(), unspent_.end(), [](std::shared_ptr<TransactionSummary> const &a, std::shared_ptr<TransactionSummary> const &b) {
          return (a->fee)  > (b->fee);
        });
      break;
    case 3:
      std::random_shuffle(unspent_.begin(), unspent_.end());
      break;
    }
    
        
    batch_size = std::min(batch_size, unspent_.size());

    
    lane_count_ = lane_count;
    batch_size_ = batch_size;

    // Adjusting the penalty  such that it is always higher than the highest
    // possible earning
    for(std::size_t i=0; i < batch_size; ++i) {
      penalty = std::max( 1 + unspent_[i]->fee, penalty);
    }

    // Computing collissions
    std::vector< std::vector< std::size_t > > group_collisions;
    group_collisions.resize( lane_count_ );
    
    for(std::size_t i=0; i < batch_size_; ++i) {
      auto &tx = unspent_[i];

      for(auto &g: tx->groups) {

        // TODO: (EJF) Need to integrate the lane resource mapping
        TODO_FAIL("Need to integrate the lane resource mappinng here");
        (void)g;
        std::size_t const group_index = 0; // TBD

        group_collisions[group_index].push_back(i);
      }
    }
    
    std::vector< int > group_matrix;
    group_matrix.resize(batch_size * batch_size);
    for(auto &b: group_matrix) b = 0;

    for(std::size_t i=0; i < lane_count_; ++i) {
      
      auto &lane = group_collisions[i];
      for(std::size_t j=0; j < lane.size(); ++j) {
        auto &a = lane[j];
        assert( a < batch_size_);
        
        for(std::size_t k=j+1; k < lane.size(); ++k) {
          auto &b = lane[k];
          assert( b < batch_size_);

          group_matrix[a *batch_size  + b] = 1;
          group_matrix[b *batch_size  + a] = 1;
        }
      }
    }

    std::size_t k = 0;
    annealer_.Reset();
    
    annealer_.Resize(batch_size);
    double max_fee = 0;
    
    for(std::size_t i=0; i < batch_size; ++i) {
      if( unspent_[i]->fee > max_fee)  max_fee =  unspent_[i]->fee; 
    }   

    
    for(std::size_t i=0; i < batch_size; ++i) {
      annealer_.Insert(i,i, int(-unspent_[i]->fee) );
      
      for(std::size_t j=0; j < batch_size; ++j) {
        if( (i<j)  && (group_matrix[k]==1) ) {
          annealer_.Insert(i,j, int( 2*max_fee ));
        }        
        ++k;
      }
    }
    
    annealer_.Normalise();    
  }    

  
  void GenerateBlockSlice() 
  {
    state_energy_ = annealer_.FindMinimum(state_);
    
    if( state_energy_ < best_solution_energy_ ) {
      best_solution_energy_ = state_energy_;
      best_solution_ = state_;
    }
  }

  std::vector< std::vector< uint64_t > > block_;
  std::vector< double > block_fees_;
  double occupancy_;
  
  std::size_t lane_count_ = 0,  batch_size_ = 0;
  double best_solution_energy_ = 0, state_energy_ = 0;  
  std::vector< int16_t > state_, best_solution_;  
  annealer_type annealer_;

  // TODO: Switch to block
//  std::shared_ptr<block_type> current_block_;

  std::unordered_map<byte_array::ConstByteArray, shared_transaction_type,
                     hasher_type> all_;
  std::vector<shared_transaction_type> unspent_, staged_;

};

}
}

#endif
