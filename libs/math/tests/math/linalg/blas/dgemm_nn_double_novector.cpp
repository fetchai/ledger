#include<gtest/gtest.h>

#include"math/linalg/prototype.hpp"
#include"math/linalg/matrix.hpp"
#include"math/linalg/blas/base.hpp"
#include"math/linalg/blas/dgemm_nn_double_novector.hpp"

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
TEST(blas_DGEMM, blas_dgemm_nn_double_novector) {

	Blas< double, Computes( _C <= _alpha * _A * _B + _beta * _C ),platform::Parallelisation::NOT_PARALLEL> dgemm_nn_double_novector;
	// Compuing _C <= _alpha * _A * _B + _beta * _C  

  double alpha = 1, beta = 0;

  Matrix< double > A = Matrix< double >(R"(
	0.30169185364333373 0.8945423972470693;
 0.04466903170163927 0.3314110217654498;
 0.3985719812230776 0.34247532861166874
	)");

  Matrix< double > B = Matrix< double >(R"(
	0.5514786044649475 0.18220489074321267 0.9295365942778178;
 0.12886903952316242 0.3914826500907078 0.13624108034372373
	)");

  Matrix< double > C = Matrix< double >(R"(
	0.7858433367659107 0.4623268657812528 0.8402115432496992;
 0.41164803091772795 0.6093238489388062 0.7242938970398791;
 0.5138165945307044 0.9067201443135698 0.37035401914270616
	)");

  Matrix< double > R = Matrix< double >(R"(
	0.2816554219716459 0.4051675595239783 0.4023070407711915;
 0.06734263532792391 0.1378805811108098 0.08667329524077186;
 0.26393838664229724 0.20669491352763875 0.4171464507617874
	)");
  
	dgemm_nn_double_novector(alpha, A, B, beta, C);

//  std::cout << C << std::endl << std::endl;
//  std::cout << R << std::endl << std::endl;  

	ASSERT_TRUE( R.AllClose(C) ); 
}