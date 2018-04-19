#ifndef OPTIMISERS_CHAIN_HPP
#define OPTIMISERS_CHAIN_HPP
#include"assert.hpp"

#include"crypto/fnv.hpp"
#include"byte_array/const_byte_array.hpp"
#include"memory/rectangular_array.hpp"

#include"chain/block.hpp"
#include"chain/transaction.hpp"
#include"chain/consensus/proof_of_work.hpp"

#include"optimisation/simulated_annealing/reference_annealer.hpp"
#include"optimisation/simulated_annealing/sparse_annealer.hpp"
#include"optimisation/instance/binary_problem.hpp"

#include"commandline/vt100.hpp"

#include<unordered_map>
#include<map>
#include<memory>
#include<algorithm>

namespace fetch {
namespace chain {


class BlockGenerator {
public:
  typedef std::shared_ptr< TransactionSummary > shared_transaction_type;

  typedef crypto::CallableFNV hasher_type;  
  // Block defs  
  typedef fetch::chain::consensus::ProofOfWork proof_type;
  typedef fetch::chain::BlockBody block_body_type;
  typedef typename proof_type::header_type block_header_type;
  typedef fetch::chain::BasicBlock<  proof_type, fetch::crypto::SHA256 > block_type;
  typedef std::shared_ptr< block_type > shared_block_type;
  
  
  void PushTransactionSummary(TransactionSummary const &tx )  {
    shared_transaction_type stx = std::make_shared< TransactionSummary >( tx );

    if(all_.find( stx->transaction_hash ) != all_.end()) return;
   
    
    all_[ stx->transaction_hash ] =  stx ;
    
    unspent_.push_back( stx );
    ++txcounter_;
  }

  double GenerateBlock(fetch::chain::BlockBody &body, std::size_t size, bool randomise = true) {
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    size = std::min( size, unspent_.size() );
    if(size == 0) {
       fetch::logger.Highlight("NOTHING TO MINE - NOTHING TO MINE - NOTHING TO MINE - NOTHING TO MINE - NOTHING TO MINE - NOTHING TO MINE ");
      return -1;
    }
    

    if(randomise) std::random_shuffle(unspent_.begin(), unspent_.end());
    
    std::vector< std::vector< std::size_t > > groups_collision;

    std::cout << " --------============== A =================--------------" << std::endl;
    
    groups_collision.resize(group_count_);
    for(std::size_t i=0; i < size; ++i) {
      auto &groups = unspent_[i]->groups;

      
      for(auto &g: groups) {
        if(g >= groups_collision.size() ) {
          TODO_FAIL("Group is out of bounds: ",g, " ", group_count_, " ", groups_collision.size() );
        }

        // TODO: Probably inefficient
        bool add = true;
        
        for(auto &c: groups_collision[g]) {
          add &= (c != i);
        }
        
        if(add) {
          groups_collision[g].push_back( i );
        }
        
      }
    }

    std::cout << " --------============== B =================--------------" << std::endl;
    
    fetch::optimisers::BinaryProblem problem; 

    problem.Resize(size);
    double field = -1 ;
    double penalty = 5. ;
    for(std::size_t i=0; i < size; ++i) {
      problem.Insert(i,i, field);
    }    
    std::cout << " --------============== C =================--------------" << std::endl;    
    for(std::size_t i=0; i < group_count_; ++i) {
      auto &group = groups_collision[i];
      //      std::cout << "Group " << i << ": ";
      
      for(std::size_t j=0; j < group.size(); ++j) {

        std::size_t A = group[j];
        //        std::cout << A << " ";        
        for(std::size_t k=j+1; k < group.size(); ++k) {
          std::size_t B = group[k];
          problem.Insert( A, B, penalty);
        }
      }
      //      std::cout << std::endl;
      
    }
    std::cout << " --------============== D =================--------------" << std::endl;    
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    static fetch::optimisers::ReferenceAnnealer annealer; // TODO: change with sparse annealer
    problem.ProgramSpinGlassSolver(annealer);
    annealer.SetSweeps( 100 );
    std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
    std::vector< int8_t > state;
    std::cout << " --------============== E =================--------------" << std::endl;
        
    auto e =  annealer.FindMinimum( state, true );

    
    std::vector< std::size_t > used_groups;
    std::size_t ncount = 0;
    
    if(state.size() != size ) {
      TODO_FAIL("Unexpected state size: ", state.size(), " <> ", size );
    }

    std::cout << "Used " << size << " transactions on " <<  group_count_ << " groups. Energy was " << e << " " << problem.energy_offset()  <<  std::endl;
    //    annealer.PrintGraph();
    std::cout << "State: " ;
    
    for(auto &s: state) {
      std::cout << int(s) <<  " ";
    }

    std::cout << std::endl;
    
    std::cout << "Active txs: " ;
    for(std::size_t i=0; i < size; ++i) {
      auto s = state[i];
      if(s == 1) {
        for(auto &g: unspent_[i]->groups) {
          used_groups.push_back(g);
        }
        std::cout << i << " ";
        ++ncount;
      }
    }
    std::chrono::high_resolution_clock::time_point t4 = std::chrono::high_resolution_clock::now();
    std::cout << std::endl << std::endl;
    double ts1 =  std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
    std::cout << "Define " << ts1*1000 << " ms" << std::endl;
    double ts2 =  std::chrono::duration_cast<std::chrono::duration<double>>(t3 - t2).count();
    std::cout << "Program " << ts2*1000 << " ms" << std::endl;
    double ts3 =  std::chrono::duration_cast<std::chrono::duration<double>>(t4 - t3).count();
    std::cout << "Solve " << ts3*1000 << " ms with " <<  annealer.sweeps() << " sweeps" << std::endl;
    //    std::cout << "Flip rate: " << annealer.accepted() / annealer.attempts() << std::endl;
    
    std::cout << std::endl << "Total: " << ncount << std::endl;
    std::cout << std::endl << "Groups: ";
    std::sort( used_groups.begin(), used_groups.end() );
    bool valid = true;
    int last_group = -1;

    std::unordered_map< uint64_t, int > groups_with_conflicts;
    std::size_t group_occupation = 0;
    
    for( auto &g: used_groups) {
      ++group_occupation;
      std::cout << g << " ";
      // TODO: Take into account malformed transactions
      valid &= (last_group < int(g) );
      if(last_group >= int(g) ) {
        --group_occupation;
        std::cout << "***"; // TODO: Correct to make valid
        if(groups_with_conflicts.find(g) == groups_with_conflicts.end()) {
          groups_with_conflicts[g] = 0;
        }

        ++groups_with_conflicts[g];                
      }
      last_group = int(g);
    }
    std::cout << std::endl;
    std::cout << "Group occupation: " << group_occupation << std::endl;
    
    if(!valid) {
      std::cout << "FIXING it" << std::endl;
      
      used_groups.clear();      
      for(std::size_t i=0; i < size; ++i) {
        auto &s = state[i];
        if(s == 1) {
          for(auto &g: unspent_[i]->groups) {
            auto it = groups_with_conflicts.find(g);
            if(it != groups_with_conflicts.end()) {
              --groups_with_conflicts[g];
              if(groups_with_conflicts[g] <= 0) {
                groups_with_conflicts.erase(it);
              }
            }
            s = 0;

            break;            
          }
          if(s == 0) break;
          
          for(auto &g: unspent_[i]->groups) {
            used_groups.push_back(g);
          }
          std::cout << i << " ";
          ++ncount;
        }
      }
      valid = true;
      
      last_group = -1;
      for( auto &g: used_groups) {
        std::cout << g << " ";
        // TODO: Take into account malformed transactions
        valid &= (last_group < int(g) );
        if(last_group >= int(g) ) {
          std::cout << "***"; // TODO: Correct to make valid
          
          ++groups_with_conflicts[g];                
        }
        last_group = int(g);
      }    
    }
    


    
    if(valid) {
      std::cout << " --------============ SOLUTION FOUND =============------------ " << std::endl;
    }
    std::cout << std::endl;

    if(valid) {
    for(std::size_t i=0; i < size; ++i) {
      auto s = state[i];
      if(s == 1) {
        body.transactions.push_back( *unspent_[i] );
        
        ++ncount;
      }
    }      
    }
    
    
    return used_groups.size();
  }
  
  void PrintTransactionSummary(TransactionSummary const &tx) {
    for(std::size_t i=0; i < group_count_; ++i) {
      for(std::size_t j=0; j < 2; ++j) {
        //        std::cout << " ";
      }
    }
  }

  void PrintBlock(block_type const& block) {
    /*
    for(std::size_t i=0; i < block.transactions.size(); ++i){
      PrintTransactionSummary(*block.transactions[i]);
    }
    */
  }

  void SwitchBranch( std::shared_ptr< block_type > new_block) 
  {
    if(!current_block_) {
      current_block_ = new_block;
      return;      
    }
    
    std::shared_ptr< block_type > old_block = current_block_;
    std::shared_ptr< block_type > end_point = new_block;
    std::unordered_set< byte_array::ConstByteArray, hasher_type > used;
    
    while( old_block && ( old_block->block_number() > new_block->block_number() ) ) {
      for(auto &tx: old_block->body().transactions) {
        PushTransactionSummary(tx);        
      }
      
      old_block = old_block->previous();      
    }

    while( (end_point)  && (old_block->block_number() < end_point->block_number()) ) {
      
      for(auto &tx: end_point->body().transactions) { 
        used.insert(tx.transaction_hash);
      }
      end_point = end_point->previous();
    }


    
    while( (old_block) && (end_point)  && (old_block != end_point) ) {
      for(auto &tx: old_block->body().transactions) {
        PushTransactionSummary(tx);
      }
      
      for(auto &tx: end_point->body().transactions) {
        used.insert(tx.transaction_hash);
      }
      
      end_point = end_point->previous();
      old_block = old_block->previous();            
    }

    if(end_point != old_block) {
      TODO_FAIL("Not suppose to happen");      
    }

    std::vector< std::size_t > to_delete;
    
    for(std::size_t i = 0; i < unspent_.size(); ++i) {
      if( used.find( unspent_[i]->transaction_hash ) != used.end() ) {
        to_delete.push_back(i);
      }
    }

    
    while(!to_delete.empty()) {
      std::size_t i = to_delete.back();
      std::swap( unspent_[i], unspent_.back() );
      to_delete.pop_back();
      unspent_.pop_back();      
    }

    current_block_ = new_block;

    // TODO: Debug code
    /*
    used.clear();
    while(new_block) {
      for(auto &tx: new_block->body().transactions) {
        if(used.find(tx.transaction_hash) !=used.end()) {
          std::cout << "Block number: " << new_block->block_number() << " / " << current_block_->block_number() << std::endl;
          
          for(auto &tx: new_block->body().transactions) {
            std::cout << byte_array::ToBase64( tx.transaction_hash) << std::endl;
            
          }
          
          TODO_FAIL("FAILED 1");          
        }
        
        used.insert(tx.transaction_hash);
      }

      new_block = new_block->previous();
      
    }
    
    for(auto &g: used) {
      std::cout << " > " << byte_array::ToBase64(g) << std::endl;
    }
    
    for(auto &s: unspent_) {
      std::cout << " - " << byte_array::ToBase64(s->transaction_hash) << std::endl;
      if(used.find(s->transaction_hash) != used.end()) {
        TODO_FAIL("FAILED 2");
      }
    }   
    */
  }
  
  
  void set_group_count(std::size_t const &g) {
    group_count_ = g;
  }
  std::vector< shared_transaction_type > const& unspent() const { return unspent_; };  
private:
  std::shared_ptr< block_type > current_block_;
  
  std::size_t group_count_ = 1;
  std::unordered_map< byte_array::ConstByteArray, shared_transaction_type, hasher_type > all_;  
  std::vector< shared_transaction_type> unspent_;


  std::size_t txcounter_ = 0;
};


inline std::ostream& operator<< (std::ostream& stream, BlockGenerator const &graph )
{
  return stream;
  
}  

};
};

#endif
