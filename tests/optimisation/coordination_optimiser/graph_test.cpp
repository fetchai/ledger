#include"optimisation/chain_group_optimiser/graph.hpp"
#include"byte_array/encoders.hpp"
#include"random/lfg.hpp"
#include"crypto/sha256.hpp"
#include"crypto/hash.hpp"
#include"byte_array/encoders.hpp"
#include<iostream>
using namespace fetch::optimisers;

std::vector< std::string > words = {"squeak", "fork", "governor", "peace", "courageous", "support", "tight", "reject", "extra-small", "slimy", "form", "bushes", "telling", "outrageous", "cure", "occur", "plausible", "scent", "kick", "melted", "perform", "rhetorical", "good", "selfish", "dime", "tree", "prevent", "camera", "paltry", "allow", "follow", "balance", "wave", "curved", "woman", "rampant", "eatable", "faulty", "sordid", "tooth", "bitter", "library", "spiders", "mysterious", "stop", "talk", "watch", "muddle", "windy", "meal", "arm", "hammer", "purple", "company", "political", "territory", "open", "attract", "admire", "undress", "accidental", "happy", "lock", "delicious"}; 

#define GROUPS 4

std::vector< std::vector< fetch::byte_array::ByteArray > > group_blocks;

fetch::random::LaggedFibonacciGenerator<> lfg;
fetch::byte_array::ByteArray RandomTX(std::size_t const &n = 161) {
  std::string ret = words[ lfg() & 63 ];
  for(std::size_t i=1; i < n;++i) {
    ret += "_" + words[lfg() & 63];
  }
  auto ret2  = fetch::byte_array::ToBase64( fetch::crypto::Hash< fetch::crypto::SHA256 >(ret) ).SubArray(0, 16);
  


  
  return ret2;
}

uint32_t CreateBlock(GroupGraph &graph, std::size_t const &n, bool has_dep = false, std::size_t const &m = 10) {
  if( (n >= graph.width()) ) {
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
    std::size_t i = lfg() % graph.width();
    if(groups.find( i ) == groups.end()) groups.insert(i);
  }
  
  std::vector< fetch::byte_array::ConstByteArray > previous;

  std::size_t block_number = 0, min_size = 10000000000000;  
  for(uint64_t g=0; g < GROUPS; ++g) {
    block_number = std::max( block_number, group_blocks[g].size() );
  }
  
  for(auto &g: groups) {    
    min_size = std::min( min_size, group_blocks[g].size() );        
  }



  std::size_t Q =  std::min(min_size, m);
  std::cout << "MIN = " << Q << std::endl;
  
  if( has_dep && (Q!= 0) ) {

    std::size_t q = lfg() % Q;
    
    for(auto &g: groups) {
      
      auto &gb = group_blocks[g];
      std::size_t j = gb.size() - 1 - q;      
      if(gb.size() > 0 ) {
        previous.push_back( gb[j] );
      }
    }
  }

  for(uint64_t g=0; g < GROUPS; ++g) {
    if(groups.find(g) == groups.end()) {
      while((group_blocks[g].size()!=0) && (group_blocks[g].size() < block_number +1)) {
        group_blocks[g].push_back( group_blocks[g].back() );        
      }
    } else {
      std::cout << " --- Adding hash: " << hash << std::endl;
      
      while(group_blocks[g].size() < block_number +1) {
        group_blocks[g].push_back( hash );
      }
      
    }
  }
  
  for(std::size_t i=0; i < block_number + 1; ++i) {
    std::cout<< std::setw(3) << i << " ";
    
    for(std::size_t j = 0; j < GROUPS; ++j) {
      if(i < group_blocks[j].size())
        std::cout << std::setw(3) << group_blocks[j][i] << " ";
      else
        std::cout << std::setw(3) << '-' << " ";              
    }
        std::cout << std::endl;
      
  }
  
  
  auto ret = graph.AddBlock(12., hash, previous, groups);  
  return ret;
}

int main() 
{
  GroupGraph graph(16, 4);
  group_blocks.resize(4);
  
    
  // Boundary condition
  /*
  auto a = CreateBlock(graph, 2, false);
  auto b = CreateBlock(graph, 1, false);
  auto c = CreateBlock(graph, 1, false);
  auto d = CreateBlock(graph, 1, false);
  */
  // Following blocks
  std::vector< uint64_t > blocks;


  std::cout << "Creating extra blocks" << std::endl;  
  for(std::size_t i=0; i < 16; ++i) {
    blocks.push_back(CreateBlock(graph, 1, true, 1));
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
  
  for(auto &e: blocks) 
  {    
    if(!graph.Activate(e)) {
      std::cout << "FAILED!!!!" << std::endl;
      
    }
    
  }
  
  // */
  
  std::cout << "Next blocks " << graph.next_blocks().size() << std::endl;
  
  std::cout << graph << std::endl;  
  return 0;  
}
