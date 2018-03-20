#include"optimisation/chain_group_optimiser/graph.hpp"
#include<iostream>
using namespace fetch::optimisers;


int main() 
{
  GroupGraph graph(16, 4);
  uint64_t a = graph.AddBlock(0, "helloworld", 12., {1,2});
  if(graph.Activate(0, a)) {
    std::cout << "OK" << std::endl;    
  }

  if(graph.Activate(0, a)) {
    std::cout << "NOT SO OK" << std::endl;    
  }
  
  
  uint64_t b = graph.AddBlock(0, "helloworldx", 12., {1,3});  
  if(graph.Deactivate(0, a)) {
    std::cout << "OK" << std::endl;    
  }

  if(graph.Activate(0, b)) {
    std::cout << "OK" << std::endl;    
  }

  uint64_t c = graph.AddBlock(0, "helloworld1", 12., {0,3});
  if(graph.Activate(0, c)) {
    std::cout << "OK" << std::endl;    
  }


  
  graph.AddBlock(1, "helloworld2", 12., {0});
  graph.AddBlock(1, "helloworld3", 12., {2});
  graph.AddBlock(1, "helloworld4", 12., {3});
  graph.AddBlock(1, "helloworld5", 12., {1});
  
  std::cout << graph << std::endl;  
  return 0;  
}
