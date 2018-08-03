#include<gtest/gtest.h>

#include"math/linalg/prototype.hpp"
#include"math/linalg/matrix.hpp"
#include"math/linalg/blas/base.hpp"
#include"math/linalg/blas/dgemm_nn_double_novector.hpp"

using namespace fetch;
using namespace fetch::math::linalg;

std::ostream& operator<<(std::ostream& os, Matrix< double > const & d)  
{  
  os << d.height() << " " << d.width() << std::endl;
  for(std::size_t i=0; i < d.height(); ++i) {
    for(std::size_t j=0; j < d.width(); ++j)  {
      os << d(j, i) <<" ";
    }
    os << std::endl;
  }

  return os;  
}  

TEST(blas_DGEMM, blas_dgemm_nn_double_novector) {

  Blas< double, Computes( _C <= _alpha * _A * _B + _beta * _C ),platform::Parallelisation::THREADING> dgemm_nn_double_novector;

        
	// Compuing _C <= _alpha * _A * _B + _beta * _C
  double alpha = 1, beta = 0;
	Matrix< double > A = Matrix< double >(R"(
	0.47448196842021717 0.8578992562624433;
 0.39501626898313924 0.46685015232059635;
 0.5509845677604795 0.6620939034839237
	)");

	Matrix< double > B = Matrix< double >(R"(
	0.5228221825957788 0.08903332737716774 0.9965283250813561;
 0.12748197003735073 0.9121745478867886 0.22274946196232892
	)");

	Matrix< double > C = Matrix< double >(R"(
	0.40080868622416077 0.8572351238262121 0.1601332420651449;
 0.5588299226771567 0.19792373261190288 0.6556725891915544;
 0.9041654512846383 0.06374249728055537 0.6989787907523035
	)");

	Matrix< double > R = Matrix< double >(R"(
	0.3574363856137136 0.8247985746425267 0.6639313190214453;
 0.26603824504067297 0.4610184394196016 0.4976355211560983;
 0.37247198945897586 0.6530011964702332 0.6965527892256078
	)");
  
	dgemm_nn_double_novector(alpha, A, B, beta, C);

  std::cout << C << std::endl << std::endl;
  std::cout << R << std::endl << std::endl;  

	ASSERT_TRUE( R.AllClose(C) ); 
}
