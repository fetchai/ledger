#include<iostream>

#include<optimisation/simulated_annealing/reference_annealer.hpp>
// #include<optimisation/instance/random_instance.hpp>
#include<optimisation/instance/load_txt.hpp>


using namespace fetch::optimisers;

int main(int argc, char **argv) {
  ReferenceAnnealer anneal;
  if(argc !=2) {
    std::cout << "usage: " << argv[0] << " [filename]" << std::endl;
    exit(-1);
  }

  Load(anneal, argv[1]);
  int counter = 0;
  for(std::size_t i=0; i < 1000; ++i) {
    //ReferenceAnnealer::state_type state;
    auto E = anneal.FindMinimum() ;
    std::cout << E << std::endl;
    if(E == -171) ++counter;
    //std::cout << anneal.CostOf(state, false)  << std::endl;
  }
  std::cout << counter << std::endl;
  return 0;
}
