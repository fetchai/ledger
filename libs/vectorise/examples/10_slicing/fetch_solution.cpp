#include"vectorise/memory/shared_array.hpp"
#include"vectorise/memory/array.hpp"
#include<iostream>
#include<chrono>
#include<vector>

typedef double type;
typedef fetch::memory::SharedArray< type > array_type;
typedef typename array_type::vector_register_type vector_type;

void SlicedOperations(array_type const &A, array_type &C) 
{
  auto aslice = A.slice(8,6) ;
  C.slice(2,6).in_parallel().Apply([](vector_type const& a, vector_type &c) {
      c = a;      
    }, aslice );  
}


int main(int argc, char const **argv) 
{
  if(argc != 2) {
    std::cout << std::endl;    
    std::cout << "Usage: " << argv[0] << " [array size] " << std::endl;
    std::cout << std::endl;
    return 0;    
  }

  std::size_t N = std::size_t(atoi(argv[1]));
  array_type A(N), C(N);    
  
  for(std::size_t i=0; i < N; ++i) {
    A[i] = double(i);
    C[i] = 0;
  }
  

  std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
  //  for(std::size_t i = 0; i < 10000; ++i) { 
  SlicedOperations(A,C);
    //  }
  for(std::size_t i=0; i < N; ++i) {
    std::cout << A[i] << " ";
  }
  std::cout << std::endl;
  for(std::size_t i=0; i < N; ++i) {
    std::cout << C[i] << " ";
  }
  std::cout << std::endl;
  
  std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
  double time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();

  std::cout << time_span << " s" << std::endl;

 
  return 0;  
}

