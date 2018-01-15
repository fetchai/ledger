#include<optimisation/simulated_annealing/reference_annealer.hpp>
#include<optimisation/instance/load_txt.hpp>

#include<iostream>

using namespace ailib::optimisers;

int main(int argc, char **argv) {
  ReferenceAnnealer anneal;
  if(argc !=2) {
    exit(-1);
  }

  Load(anneal, argv[1]);
  int counter = 0;
  for(std::size_t i=0; i < 1000; ++i) {
    auto E = anneal.FindMinimum() ;
    std::cout << E << std::endl;
  }

  return 0;
}
