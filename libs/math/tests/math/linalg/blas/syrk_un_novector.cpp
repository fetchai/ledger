#include<gtest/gtest.h>

#include"math/linalg/prototype.hpp"
#include"math/linalg/matrix.hpp"
#include"math/linalg/blas/base.hpp"
#include"math/linalg/blas/dsyrk_un_novector.hpp"


using namespace fetch;
using namespace fetch::math::linalg;

TEST(blas_A_withA, blas_dsyrk_un_novector1) {

	Blas< double,
          Signature(( U(_C) <= _alpha, U(_A), _beta, U(_C) )),
          Computes( _C = _alpha * _A * T(_A) + _beta * _C ),
          platform::Parallelisation::NOT_PARALLEL > dsyrk_un_novector;
	// Compuing _C = _alpha * _A * T(_A) + _beta * _C  

  double alpha = double(1), beta = double(0);

Matrix< double > A = Matrix< double >(R"(
	0.3745401188473625 0.9507143064099162;
 0.7319939418114051 0.5986584841970366;
 0.15601864044243652 0.15599452033620265
	)");



Matrix< double > C = Matrix< double >(R"(
	0.05808361216819946 0.8661761457749352 0.6011150117432088;
 0.7080725777960455 0.020584494295802447 0.9699098521619943;
 0.8324426408004217 0.21233911067827616 0.18182496720710062
	)");

  Matrix< double > R = Matrix< double >(R"(
	1.0441379930386843 0.8433142835413905 0.20674146233889457;
 0.8433142835413905 0.8942071115496921 0.20759214270103027;
 0.20674146233889457 0.20759214270103027 0.04867610654042823
	)");
  
	dsyrk_un_novector(alpha, A, beta, C);

	ASSERT_TRUE( R.AllClose(C) ); 
}
