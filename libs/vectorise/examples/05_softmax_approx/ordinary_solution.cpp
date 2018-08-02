#include<iostream>
#include<vector>
#include<chrono>
#include<cmath>

typedef float type;
typedef std::vector< type > array_type;

void SoftMax(array_type const &A, array_type &B) 
{
  type sum = 0;
  
  for(std::size_t i = 0; i < A.size(); ++i) {
    type e = type( std::exp( A[i] ) );    
    B[i] = e;    
    sum += e;
  }

  sum = type(1./sum);  
  for(std::size_t i = 0; i < A.size(); ++i) {
    B[i] *= sum;    
  }
}

int main(int argc, char const **argv) 
{
  if(argc!=2) {
    std::cout << std::endl;    
    std::cout << "Usage: " << argv[0] << " [array size] " << std::endl;
    std::cout << std::endl;
    return 0;    
  }
  std::size_t N = std::size_t(atoi(argv[1]));
  
  std::vector< type > A, B;

  A.resize(N);
  B.resize(N);

  for(std::size_t i=0; i < N; ++i) {
    A[i] = type(std::sin( 0.1 * type(i) ));
  }
  
  std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
  for(std::size_t i = 0; i < 10000; ++i) {  
    SoftMax(A, B);
  }
  std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now(); 
  double time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();

  std::cout << time_span << " s giving " << B[0] << " for first element" << std::endl;  
  
  return 0;  
}
