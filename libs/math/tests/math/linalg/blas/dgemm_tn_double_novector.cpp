#include<gtest/gtest.h>

#include"math/linalg/prototype.hpp"
#include"math/linalg/matrix.hpp"
#include"math/linalg/blas/base.hpp"
#include"math/linalg/blas/dgemm_tn_double_novector.hpp"

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
TEST(blas_DGEMM, blas_dgemm_tn_double_novector) {

	Blas< double, Computes( _C <= _alpha * T(_A) * _B + _beta * _C ),platform::Parallelisation::NOT_PARALLEL> dgemm_tn_double_novector;
	// Compuing _C <= _alpha * T(_A) * _B + _beta * _C  

  double alpha = 1, beta = 0;

  Matrix< double > A = Matrix< double >(R"(
	0.5121541851516425 0.8708423031430542 0.4306416555796374;
 0.40423309906534044 0.43197095100393634 0.15517006293663016
	)");

  Matrix< double > B = Matrix< double >(R"(
	0.7914464943051144 0.6874763057353065 0.2265626890300917;
 0.10870179123261015 0.6583229112924786 0.9171000520808523
	)");

  Matrix< double > C = Matrix< double >(R"(
	0.035870252797329694 0.17620746124261533 0.8597127884816084;
 0.934720252052361 0.3005434857730499 0.7135939197441533;
 0.2576455652096844 0.7394485948289052 0.8002849990402334
	)");

  Matrix< double > R = Matrix< double >(R"(
	0.44928349632587156 0.6182097777924032 0.4867572255915996;
 0.7361811040497439 0.8830598235015048 0.593460955584374;
 0.3576970923971431 0.3982079420515049 0.2398738042970659
	)");
  
	dgemm_tn_double_novector(alpha, A, B, beta, C);

//  std::cout << C << std::endl << std::endl;
//  std::cout << R << std::endl << std::endl;  

	ASSERT_TRUE( R.AllClose(C) ); 
}