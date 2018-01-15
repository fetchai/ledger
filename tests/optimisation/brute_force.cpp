#include<iostream>

#include<optimisation/brute_force.hpp>
#include<random/lfg.hpp>


fetch::optimisation::BruteForceOptimiser<double> GenerateProblem(bool field) {
  static fetch::random::LinearCongruentialGenerator gen;
  fetch::optimisation::BruteForceOptimiser<double> ret( ( (gen() )% 16) );
  if(ret.size() == 0 ) return ret;

  std::size_t N =  (gen() % ret.size()) ;
  N *= N;

  for(std::size_t k=0; k < N; ++k) {
    std::size_t i = gen() % ret.size();
    std::size_t j = gen() % ret.size();
    if(i == j ) continue;
    double value = 1. - 2. * gen.AsDouble();
    ret(i, j) = value;
  }

  if(field) {
    for(std::size_t k=0; k < ret.size(); ++k) {
      double value = 1. - 2. * gen.AsDouble();
      ret(k,k) = value;
    }
  }

  return ret;
}

void SimpleTest() {
  fetch::optimisation::BruteForceOptimiser<double> solver(29);

  solver(1,0) = 0.5;
  solver(0,2) = -1.0;
  solver(2,3) = 1.2;
  solver(1,3) = -0.3;
  uint64_t ret;
  std::cout << solver.FindMinimum(ret) << std::endl;
  std::cout << ret << std::endl;
  std::cout << solver.CostOf(ret) << std::endl;
}

bool SingleTest(bool field = false) {
  auto solver = GenerateProblem(field);
  uint64_t ret;
  double d1 = solver.FindMinimum(ret) ;
  double d2 = solver.CostOf(ret);
  if((d2!=0) && (fabs( (d1- d2) / d2 ) > 1e-10 )) {
    std::cout << "cost of does not work" << std::endl;
    return false;
  }
  double min = 10000000;
  uint64_t best = 0;
  for(std::size_t i = 0 ; i < (1 << solver.size() ); ++i) {
    double cand = solver.CostOf(i);
    if( cand < min ) {
      min = cand  ;
      best = i;
    }
  }

  if( (min == 0) || ( fabs( (min - solver.CostOf(ret) )/min) < 1e-10) ) return true;

  std::cout << "wrong result with no field "<<std::endl;
  std::cout << best << " " << ret << std::endl;
  std::cout << min << " " << solver.CostOf(ret) << std::endl;
  return false;
}

int main() {
  std::cout << "Testing no field" << std::endl;
  for(std::size_t i=0; i < 1000; ++i) 
    if(!SingleTest(false)) exit(-1) ;    

  std::cout << "Testing with field" << std::endl;
  for(std::size_t i=0; i < 1000; ++i) 
    if(!SingleTest(true)) exit(-1) ;    


  return 0;
}
