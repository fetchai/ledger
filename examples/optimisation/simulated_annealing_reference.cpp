#include<optimisation/simulated_annealing/reference_annealer.hpp>
#include<optimisation/simulated_annealing/sparse_annealer.hpp>
#include<optimisation/instance/load_txt.hpp>
#include<optimisation/instance/binary_problem.hpp>

#include<iostream>

using namespace fetch::optimisers;
void Test1(int argc, char **argv) {
  //SparseAnnealer anneal;
  ReferenceAnnealer anneal;  
  if(argc !=2) {
    exit(-1);
  }
  
  Load(anneal, argv[1]);

  int counter = 0;
  for(std::size_t i=0; i < 3; ++i) {
    std::vector< int8_t > state;
    auto E = anneal.FindMinimum(state, false) ;
    
    std::cout << E  << " " << anneal.CostOf( state, false ) <<  std::endl;
    for(auto &s: state) {
      std::cout << int(s) << " ";
    }
    std::cout << std::endl;

    
    ReferenceAnnealer::SpinToBinary(state);
    for(auto &s: state) {
      std::cout << int(s) << " ";
    }
    std::cout << std::endl << std::endl; 
  }

}

void Test2() {
  ReferenceAnnealer anneal;
  DenseBinaryProblem problem;
  problem.Resize(4);
  problem.Insert(0, 2, 2.5);
  problem.Insert(0, 1, 2.5);
  problem.Insert(1, 3, 2.5);
  problem.Insert(0, 0, -1.0);
  problem.Insert(1, 1, -1.0);
  problem.Insert(2, 2, -1.0);
  problem.Insert(3, 3, -1.0);
  problem.ProgramSpinGlassSolver(anneal);
  anneal.PrintGraph();
  std::vector< int8_t > state;
  
  for(std::size_t i=0; i < 10; ++i) {

    auto E = anneal.FindMinimum(state, false) ;

    
    std::cout << (E+problem.energy_offset())  << std::endl;
    std::cout << (anneal.CostOf(state, false) + problem.energy_offset()) << std::endl;    
    for(auto &s: state) {
      std::cout << int(s) << " ";
    }
    std::cout << std::endl;

    
    ReferenceAnnealer::SpinToBinary(state);
    for(auto &s: state) {
      std::cout << int(s) << " ";
    }
    std::cout << std::endl;
    std::cout << std::endl;    
  }
  
  for(auto &s : state) s = 1;
  auto E = anneal.CostOf(state, false);
  std::cout << E + problem.energy_offset() << std::endl;
}

int main(int argc, char **argv) {
  Test1(argc, argv);
  
  return 0;
}
