#include<iostream>

#include<math/linalg/matrix.hpp>
#include<random/lcg.hpp>

#include<iomanip>
using namespace fetch::math::linalg;

typedef float data_type;
typedef fetch::memory::SharedArray< data_type > container_type;
typedef Matrix<data_type,container_type>  matrix_type;
typedef typename matrix_type::vector_register_type vector_register_type;


Matrix<data_type, container_type> RandomMatrix(int n, int m) {
  static fetch::random::LinearCongruentialGenerator gen;
  Matrix<data_type,container_type> m1(n, m);
  for(std::size_t i=0; i < n; ++i)
    for(std::size_t j=0; j < m; ++j)
      m1(i,j) = gen.AsDouble();
  return m1;
}


void test_invert(std::size_t const &n) {
  static fetch::random::LinearCongruentialGenerator gen;
  Matrix<data_type, container_type> m1 = RandomMatrix(n, n);

  Matrix<data_type, container_type> m2 = m1.Copy();
  int errcode = m2.Invert();

  if( errcode != Matrix<data_type,container_type>::INVERSION_OK ) {
    std::cerr << "inversion singular" << std::endl;
    exit(-1);
  }
//  m2.Transpose();
  
  Matrix<data_type,container_type> ret;
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


void add_kernel(vector_register_type const &a,
                vector_register_type const &b,
                vector_register_type &c ){
  c =   a + b;
}

void add_kernel2(data_type const &a,
                 data_type const &b,
                 data_type &c ){
  c =  a + b;
}


void test_add(std::size_t const &n, std::size_t const &m) {
  Matrix<data_type,container_type> m1 = RandomMatrix(n,m);
  Matrix<data_type,container_type> m2 = RandomMatrix(n,m);
  Matrix<data_type, container_type> ret(n,m), ret2(n,m);
  
  std::cout << m1.size() << " " << m2.size() << " " << (n*m) << std::endl;

  ret.ApplyKernelElementWise(add_kernel2, m1, m2);  
  ret = m1 + m2;
  
  for(std::size_t i=0; i < 1000; ++i)
    ret2.ApplyKernelElementWise(add_kernel2, m1, m2);
  
  for(std::size_t i=0; i < ret2.size(); ++i) {
    if(ret[i] != ret2[i]) {
      std::cout << "FAILED: " << ret[i] << " " << ret2[i] << std::endl;
      exit(-1);
    }

  }
  //  for(std::size_t i=0; i < 100; ++i)
  std::cout << "After assignment" << std::endl;
  
  // TODO: check that 
}

void test_elementwise(std::size_t const &n, std::size_t const &m) {
  Matrix<data_type,container_type> m1 = Matrix<data_type,container_type>(n,m); //RandomMatrix(n,m);
  Matrix<data_type,container_type> m2 = Matrix<data_type,container_type>(n,m);
  std::cout << m1.size() << " " << m2.size() << " " << (n*m) << std::endl;
  m2 = m1 * m1;
  //  for(std::size_t i=0; i < 100; ++i)
  std::cout << "After assignment" << std::endl;
  
  // TODO: check that 
}



int main() {
  static fetch::random::LinearCongruentialGenerator gen;

  //test_add( 2000, 2000) ; //egen() % 250, gen() % 250);
//  std::cout << "Testing invert" << std::endl;
  //  for(std::size_t i=0; i < 100; ++i)
  //    test(gen() % 250);

  
  Matrix<data_type,fetch::memory::Array< data_type > >  a(1,3), b(1,3);
  for(std::size_t i=0; i < a.size(); ++i) {
    b[i] = 1.1;
    a[i] = 1.2;
  }
  
  std::cout << a.size() << " " << b.size() << std::endl;
  
  a += b;  
  b = a.Copy();
  
  return 0;
}
