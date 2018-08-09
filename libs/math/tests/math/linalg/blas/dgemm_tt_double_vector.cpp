#include<gtest/gtest.h>

#include"math/linalg/prototype.hpp"
#include"math/linalg/matrix.hpp"
#include"math/linalg/blas/base.hpp"
#include"math/linalg/blas/dgemm_tt_double_vector.hpp"
#include"math/linalg/blas/dgemm_tt_double_vector_threaded.hpp"


using namespace fetch;
using namespace fetch::math::linalg;

TEST(blas_DGEMM_vectorised, blas_dgemm_tt_double_vector1) {

	Blas< double, Computes( _C <= _C = _alpha * T(_A) * T(_B) + _beta * _C ),platform::Parallelisation::VECTORISE> dgemm_tt_double_vector;
	// Compuing _C <= _C = _alpha * T(_A) * T(_B) + _beta * _C  

  double alpha = 1, beta = 0;

  Matrix< double > A = Matrix< double >(R"(
	0.3745401188473625 0.9507143064099162 0.7319939418114051;
 0.5986584841970366 0.15601864044243652 0.15599452033620265
	)");

  Matrix< double > B = Matrix< double >(R"(
	0.05808361216819946 0.8661761457749352;
 0.6011150117432088 0.7080725777960455;
 0.020584494295802447 0.9699098521619943
	)");

  Matrix< double > C = Matrix< double >(R"(
	0.8324426408004217 0.21233911067827616 0.18182496720710062;
 0.18340450985343382 0.3042422429595377 0.5247564316322378;
 0.43194501864211576 0.2912291401980419 0.6118528947223795
	)");

  Matrix< double > R = Matrix< double >(R"(
	0.5402983414818157 0.649035344064104 0.5883544808430341;
 0.1903605457037474 0.6819611623843438 0.17089398970327166;
 0.17763558461246698 0.5504679890644331 0.16636834727714633
	)");
  
	dgemm_tt_double_vector(alpha, A, B, beta, C);

	ASSERT_TRUE( R.AllClose(C) ); 
}

TEST(blas_DGEMM_vectorised, blas_dgemm_tt_double_vector2) {

	Blas< double, Computes( _C <= _C = _alpha * T(_A) * T(_B) + _beta * _C ),platform::Parallelisation::VECTORISE> dgemm_tt_double_vector;
	// Compuing _C <= _C = _alpha * T(_A) * T(_B) + _beta * _C  

  double alpha = 0, beta = 1;

  Matrix< double > A = Matrix< double >(R"(
	0.13949386065204183 0.29214464853521815 0.3663618432936917;
 0.45606998421703593 0.7851759613930136 0.19967378215835974
	)");

  Matrix< double > B = Matrix< double >(R"(
	0.5142344384136116 0.5924145688620425;
 0.046450412719997725 0.6075448519014384;
 0.17052412368729153 0.06505159298527952
	)");

  Matrix< double > C = Matrix< double >(R"(
	0.9488855372533332 0.9656320330745594 0.8083973481164611;
 0.3046137691733707 0.09767211400638387 0.6842330265121569;
 0.4401524937396013 0.12203823484477883 0.4951769101112702
	)");

  Matrix< double > R = Matrix< double >(R"(
	0.9488855372533332 0.9656320330745594 0.8083973481164611;
 0.3046137691733707 0.09767211400638387 0.6842330265121569;
 0.4401524937396013 0.12203823484477883 0.4951769101112702
	)");
  
	dgemm_tt_double_vector(alpha, A, B, beta, C);

	ASSERT_TRUE( R.AllClose(C) ); 
}

TEST(blas_DGEMM_vectorised, blas_dgemm_tt_double_vector3) {

	Blas< double, Computes( _C <= _C = _alpha * T(_A) * T(_B) + _beta * _C ),platform::Parallelisation::VECTORISE> dgemm_tt_double_vector;
	// Compuing _C <= _C = _alpha * T(_A) * T(_B) + _beta * _C  

  double alpha = 0.9600609930799429, beta = 0.7580431291595229;

  Matrix< double > A = Matrix< double >(R"(
	0.034388521115218396 0.9093204020787821 0.2587799816000169;
 0.662522284353982 0.31171107608941095 0.5200680211778108
	)");

  Matrix< double > B = Matrix< double >(R"(
	0.5467102793432796 0.18485445552552704;
 0.9695846277645586 0.7751328233611146;
 0.9394989415641891 0.8948273504276488
	)");

  Matrix< double > C = Matrix< double >(R"(
	0.5978999788110851 0.9218742350231168 0.0884925020519195;
 0.1959828624191452 0.045227288910538066 0.32533033076326434;
 0.388677289689482 0.2713490317738959 0.8287375091519293
	)");

  Matrix< double > R = Matrix< double >(R"(
	0.5888625113683394 1.2238637222759494 0.6672642609674754;
 0.6811630511598611 1.1127020951991287 1.334587366869804;
 0.5227586259969872 0.833603810797227 1.3084168118718278
	)");
  
	dgemm_tt_double_vector(alpha, A, B, beta, C);

	ASSERT_TRUE( R.AllClose(C) ); 
}
