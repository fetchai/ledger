#include<gtest/gtest.h>

#include"math/linalg/prototype.hpp"
#include"math/linalg/matrix.hpp"
#include"math/linalg/blas/base.hpp"
#include"math/linalg/blas/dgemm_nt_double_novector.hpp"

using namespace fetch;
using namespace fetch::math::linalg;
/*
std::ostream& operator<<(std::ostream& os, Matrix< double > const & d)  
{  
  os << d.height() << " " << d.width() << std::endl;
  for(std::size_t i=0; i < d.height(); ++i) {
    for(std::size_t j=0; j < d.width(); ++j)  {
      os << d(i, j) <<" ";
    }
    os << std::endl;
  }

  return os;  
}  
*/
TEST(blas_DGEMM, blas_dgemm_nt_double_novector) {

	Blas< double, Computes( _C <= _alpha * _A * T(_B) + _beta * _C ),platform::Parallelisation::NOT_PARALLEL> dgemm_nt_double_novector;
	// Compuing _C <= _alpha * _A * T(_B) + _beta * _C  

  double alpha = 1, beta = 0;

  Matrix< double > A = Matrix< double >(R"(
	0.11092435465113193 0.506354903066295;
 0.7236755078605477 0.11666359751433097;
 0.4169462439743197 0.8798218737143069
	)");

  Matrix< double > B = Matrix< double >(R"(
	0.14628088085481272 0.1709994282134777;
 0.6913392675596204 0.10910633058872776;
 0.34285092382476323 0.7544253644038865
	)");

  Matrix< double > C = Matrix< double >(R"(
	0.33551788645326164 0.4320464659799038 0.5071338583803412;
 0.9029447753621139 0.8828281226039523 0.6947140638868838;
 0.6576286075994151 0.9876149021883325 0.8721238298926671
	)");

  Matrix< double > R = Matrix< double >(R"(
	0.1028125112040466 0.1319328875482115 0.42003749973029053;
 0.12580929921117276 0.5130340325932162 0.336126793486761;
 0.21144030117252616 0.38424544713360803 0.8067103426192868
	)");
  
	dgemm_nt_double_novector(alpha, A, B, beta, C);

//  std::cout << C << std::endl << std::endl;
//  std::cout << R << std::endl << std::endl;  

	ASSERT_TRUE( R.AllClose(C) ); 
}