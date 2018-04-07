#include<iostream>

#include<math/linalg/matrix.hpp>
#include<random/lcg.hpp>

#include<iomanip>
using namespace fetch::math::linalg;

typedef fetch::memory::Array< double > container_type;


Matrix<double, container_type> RandomMatrix(int n, int m) {
  static fetch::random::LinearCongruentialGenerator gen;
  Matrix<double,container_type> m1(n, m);
  for(std::size_t i=0; i < n; ++i)
    for(std::size_t j=0; j < n; ++j)
      m1(i,j) = gen.AsDouble();
  return m1;
}


void test_invert(std::size_t const &n) {
  static fetch::random::LinearCongruentialGenerator gen;
  Matrix<double, container_type> m1 = RandomMatrix(n, n);

  Matrix<double,container_type> m2 = m1.Copy();
  int errcode = m2.Invert();

  if( errcode != Matrix<double,container_type>::INVERSION_OK ) { 
    std::cerr << "inversion singular" << std::endl;
    exit(-1);
  }
//  m2.Transpose();
  
  Matrix<double,container_type> ret;
  m1.DotReference( m2, ret );
  for(std::size_t i=0; i < n; ++i) {
  for(std::size_t j=0; j < n; ++j) {
    if(i==j ) {
      if( fabs(ret(i,j) -1) > 1e-10) {
        std::cerr << "Expected 1 on the diagonal, but found " << ret(i,j) << std::endl;
        exit(-1);
      }
    } else if( fabs(ret(i,j)) > 1e-10) {
        std::cerr << "Off-diagonal is non-zero" << std::endl;
        exit(-1);
      }
  }

  }
}


void test_add(std::size_t const &n, std::size_t const &m) {
  Matrix<double,container_type> m1 = Matrix<double,container_type>(n,m); //RandomMatrix(n,m);
  Matrix<double,container_type> m2 = Matrix<double,container_type>(n,m);
  std::cout << m1.size() << " " << m2.size() << " " << (n*m) << std::endl;
  m2 = m1 * m1;
  //  for(std::size_t i=0; i < 100; ++i)
  std::cout << "After assignment" << std::endl;
  
  // TODO: check that 
}


int main() {
  static fetch::random::LinearCongruentialGenerator gen;

  test_add(gen() % 250, gen() % 250);
  std::cout << "Testing invert" << std::endl;
  //  for(std::size_t i=0; i < 100; ++i)
  //    test(gen() % 250);
  

  return 0;
}
