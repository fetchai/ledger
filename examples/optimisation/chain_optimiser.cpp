#include<optimisation/simulated_annealing/reference_annealer.hpp>
#include<chain/block_generator.hpp>
#include"random/lfg.hpp"

#include"byte_array/encoders.hpp"
#include<iostream>

using namespace fetch::optimisers;

fetch::random::LaggedFibonacciGenerator<> lfg;

fetch::byte_array::ByteArray RandomTX(std::size_t const &n = 32) {
  fetch::byte_array::ByteArray ret;
  ret.Resize( n );
  
  for(std::size_t i=0; i < n ;) {
    uint64_t word = lfg() ;
    for(std::size_t j=0; j < 8; ++j, ++i) {
      ret[i] = word & 255;
      word >>= 8;
    }

  }
  return fetch::byte_array::ToBase64( ret ).SubArray(0, 8);
}

void test() {
  chain::BlockGenerator coordinator;

  std::size_t group_count = 256;
  std::size_t max_groups = 3;
  std::size_t transaction_count = 10000;
  std::size_t transactions_per_block = 256;

  std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
  for(std::size_t i=0; i < transaction_count; ++i) {
    Transaction tx;
    std::size_t groups = 2 + ( (lfg() >> 19) % max_groups );

    std::unordered_set< std::size_t > used;
    while(tx.groups.size() < groups) {
      auto g = (lfg() >> 19 ) % group_count ;
      if(used.find(g) == used.end()) {
        tx.groups.push_back( g );
        used.insert(g);
      }
    }
    tx.transaction_hash = RandomTX();
    //    std::cout << i << "  - " << groups << ": ";
    //    for(auto &g: tx.groups)
    //      std::cout << " " << g;
    //    std::cout << std::endl;
    coordinator.PushTransaction( tx );
  }
  std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
  double time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
  std::cout << "Generating " << transaction_count <<  " took " << time_span*1000 << " ms" << std::endl;

  coordinator.set_group_count(group_count);
  coordinator.MineBlock(transactions_per_block);
  std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
  time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t3 - t2).count();
  std::cout << "Finding groupds " << transaction_count <<  " took " << time_span*1000 << " ms" << std::endl;
  //  coordinator.MineBlock(transactions_per_block);
  
  std::cout << "======================" << std::endl;
  
}

int main(int argc, char **argv) {
  test();
  test();
  test();  
  
  return 0;
}
