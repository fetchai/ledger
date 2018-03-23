#include"protocols/swarm/coordination_manager.hpp"
#include"byte_array/encoders.hpp"
#include"random/lfg.hpp"
#include"crypto/sha256.hpp"
#include"crypto/hash.hpp"
#include"byte_array/encoders.hpp"
#include<iostream>
using namespace fetch::protocols;

std::vector< std::string > words = {"squeak", "fork", "governor", "peace", "courageous", "support", "tight", "reject", "extra-small", "slimy", "form", "bushes", "telling", "outrageous", "cure", "occur", "plausible", "scent", "kick", "melted", "perform", "rhetorical", "good", "selfish", "dime", "tree", "prevent", "camera", "paltry", "allow", "follow", "balance", "wave", "curved", "woman", "rampant", "eatable", "faulty", "sordid", "tooth", "bitter", "library", "spiders", "mysterious", "stop", "talk", "watch", "muddle", "windy", "meal", "arm", "hammer", "purple", "company", "political", "territory", "open", "attract", "admire", "undress", "accidental", "happy", "lock", "delicious"}; 

#define GROUPS 1024

std::vector< std::vector< fetch::byte_array::ByteArray > > group_blocks;

fetch::random::LaggedFibonacciGenerator<> lfg;
fetch::byte_array::ByteArray RandomTX(std::size_t const &n = 161) {
  std::string ret = words[ (lfg()>>19) & 63 ];
  for(std::size_t i=1; i < n;++i) {
    ret += "_" + words[(lfg() >> 20) & 63];
  }
  auto ret2  = fetch::byte_array::ToBase64( fetch::crypto::Hash< fetch::crypto::SHA256 >(ret) ).SubArray(0, 8);

  return ret2;
}

uint32_t CreateBlock(CoordinationManager &graph, std::size_t const &n, bool has_dep = false, std::size_t const &m = 10) {
  if( (n >= graph.groups()) ) {
    std::cerr << "error: " <<  n << " is too big"  << std::endl;
    exit(-1);    
  }

  if( n == 0 ) {
    std::cerr << "error: " <<  n << " cannot be zero"  << std::endl;
    exit(-1);    
  }
  
  auto hash = RandomTX();
  
  std::unordered_set< uint32_t > groups;
  while(groups.size() < n) {
    std::size_t i = (lfg()>>19) % graph.groups();
    if(groups.find( i ) == groups.end()) groups.insert(i);
  }
  
  std::unordered_map< uint32_t, fetch::byte_array::ConstByteArray > previous;

  std::size_t block_number = 0, min_size = 10000000000000;  
  for(uint64_t g=0; g < GROUPS; ++g) {
    block_number = std::max( block_number, group_blocks[g].size() );
  }
  
  for(auto &g: groups) {    
    min_size = std::min( min_size, group_blocks[g].size() );        
  }

  std::size_t Q =  std::min(min_size, m);
  if( has_dep && (Q!= 0) ) {

    std::size_t q = (lfg() >> 19) % Q;
    
    for(auto &g: groups) {
      
      auto &gb = group_blocks[g];
      std::size_t j = gb.size() - 1 - q; 
      if(gb.size() > 0 ) {
        previous[g] = gb[j];        
      }
    }
  }

  for(uint64_t g=0; g < GROUPS; ++g) {
    if(groups.find(g) == groups.end()) {
      while((group_blocks[g].size()!=0) && (group_blocks[g].size() < block_number +1)) {
        group_blocks[g].push_back( group_blocks[g].back() );        
      }
    } else {
      while(group_blocks[g].size() < block_number + 1) {
        group_blocks[g].push_back( hash );
      }
      
    }
  }
    
  auto ret = graph.AddBlock(12., hash, previous);  
  return ret;
}

uint32_t CreateGenesis(CoordinationManager &graph, std::size_t const &n) {
  if( (n >= graph.groups()) ) {
    std::cerr << "error: " <<  n << " is too big"  << std::endl;
    exit(-1);    
  }

  auto hash = RandomTX();    
  std::unordered_map< uint32_t, fetch::byte_array::ConstByteArray > previous;
  previous[n] = "genesis";
  
  
  group_blocks[n].push_back( hash );  
  
  auto ret = graph.AddBlock(0, hash, previous);
  graph.Activate(ret);
  return ret;
}

int main() 
{
  CoordinationManager graph(1200, GROUPS);
  group_blocks.resize(GROUPS);

  // Boundary condition
  for(std::size_t i=0; i < GROUPS; ++i)
    CreateGenesis(graph, i);
  
  // Following blocks
  std::vector< uint64_t > blocks;


  std::cout << "Creating extra blocks" << std::endl;  
  for(std::size_t i=0; i < 150000; ++i) {
    blocks.push_back(CreateBlock(graph, 1 + ( (lfg() >>19) % 3), true, 1));
  }
  std::cout << "Total blocks generated: " << blocks.size() << std::endl;
  
  /*
  graph.Activate(a);  
  graph.Activate(b);
  graph.Activate(c);
  if(graph.Activate(d)) {
    std::cout << "NOT OK ADDING D" << std::endl;   
  }
  */

  std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
  std::size_t count = 0;
  
  for(auto &e: blocks) 
  {    
    if(graph.Activate(e)) {
      ++count;      
    }
    
  }
  std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
  double time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
  // */
  
  std::cout << "Applying took " << time_span*1000 << " ms" << " with " << count << " successful" << std::endl;
  
//  std::cout << graph << std::endl;  
  return 0;  
}
