#ifndef OPTIMISERS_CHAIN_HPP
#define OPTIMISERS_CHAIN_HPP
#include"assert.hpp"

#include"crypto/fnv.hpp"
#include"byte_array/const_byte_array.hpp"
#include"memory/rectangular_array.hpp"

#include"chain/block.hpp"
#include"chain/consensus/proof_of_work.hpp"
#include"optimisation/instance/binary_problem.hpp"

#include"commandline/vt100.hpp"

#include<unordered_map>
#include<map>
#include<memory>
#include<algorithm>

namespace fetch {
namespace chain {


struct TransactionSummary {
  std::vector< uint32_t > groups;
  byte_array::ByteArray transaction_hash;
};

struct Block {
  // TODO: Pregvious hash
  std::vector< std::shared_ptr< TransactionSummary > > transactions;
};

class BlockGenerator {
public:
  typedef std::shared_ptr< TransactionSummary > shared_transaction_type;
  
  void PushTransactionSummary(TransactionSummary const &tx )  {
    shared_transaction_type stx = std::make_shared< TransactionSummary >( tx );

    unspent_.push_back( stx );
    ++txcounter_;
  }

  bool MineBlock(std::size_t size, bool randomise = true) {
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    size = std::min( size, unspent_.size() );
    if(size == 0) return false;

    if(randomise) std::random_shuffle(unspent_.begin(), unspent_.end());
    
    std::vector< std::vector< std::size_t > > groups_collision;

    groups_collision.resize(group_count_);
    for(std::size_t i=0; i < size; ++i) {
      auto &groups = unspent_[i]->groups;
      for(auto &g: groups) {
        groups_collision[g].push_back( i );
      }
    }
    
    optimisers::DenseBinaryProblem problem;  // TODO: Change with sparse

    problem.Resize(size);
    double field = -1;
    double penalty = 2.5;
    for(std::size_t i=0; i < size; ++i) {
      problem.Insert(i,i, field);
    }    
    
    for(std::size_t i=0; i < group_count_; ++i) {
      auto &group = groups_collision[i];
      for(std::size_t j=0; j < group.size(); ++j) {

        std::size_t A = group[j];
        for(std::size_t k=j+1; k < group.size(); ++k) {
          std::size_t B = group[k];
          problem.Insert( A, B, penalty );
        }
      }
    }

    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    ReferenceAnnealer annealer; // TODO: change with sparse annealer
    problem.ProgramSpinGlassSolver(annealer);
    annealer.SetSweeps( 10 );
    std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
    std::vector< int8_t > state;
    
    auto e = problem.energy_offset() +  annealer.FindMinimum( state, true );

    
    std::vector< std::size_t > used_groups;
    std::size_t ncount = 0;
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
    std::cout << "Flip rate: " << annealer.accepted() / annealer.attempts() << std::endl;
    
    std::cout << std::endl << "Total: " << ncount << std::endl;
    std::cout << std::endl << "Groups: ";
    std::sort( used_groups.begin(), used_groups.end() );
    bool valid = true;
    int last_group = -1;
    for( auto &g: used_groups) {
      std::cout << g << " ";
      valid &= (last_group < int(g) );
      if(last_group >= int(g) ) {
        std::cout << "***";
      }
      last_group = g;
    }
    if(valid) {
      std::cout << " --------============ SOLUTION FOUND =============------------ " << std::endl;
    }
    std::cout << std::endl;    
    return true;
  }
  
  void PrintTransactionSummary(TransactionSummary const &tx) {
    for(std::size_t i=0; i < group_count_; ++i) {
      for(std::size_t j=0; j < 2; ++j) {
        //        std::cout << " ";
      }
    }
  }

  void PrintBlock(Block const& block) {
    for(std::size_t i=0; i < block.transactions.size(); ++i){
      PrintTransactionSummary(*block.transactions[i]);
    }    
  }

  void set_group_count(std::size_t const &g) {
    group_count_ = g;
  }
private:
  std::size_t group_count_ = 1;
  std::vector< shared_transaction_type> unspent_;

  std::size_t txcounter_ = 0;
};


std::ostream& operator<< (std::ostream& stream, BlockGenerator const &graph )
{

}  

};
};

#endif
