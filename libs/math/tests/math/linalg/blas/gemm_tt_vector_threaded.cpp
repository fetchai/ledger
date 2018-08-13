#include <gtest/gtest.h>

#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/gemm_tt_vector.hpp"
#include "math/linalg/blas/gemm_tt_vector_threaded.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"

using namespace fetch;
using namespace fetch::math::linalg;

TEST(blas_gemm_vectorised, blas_gemm_tt_vector_threaded1)
{

  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C = _alpha * T(_A) * T(_B) + _beta * _C),
       platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>
      gemm_tt_vector_threaded;
  // Compuing _C = _alpha * T(_A) * T(_B) + _beta * _C

  double alpha = double(1), beta = double(0);

  Matrix<double> A = Matrix<double>(R"(
	0.3745401188473625 0.9507143064099162 0.7319939418114051;
 0.5986584841970366 0.15601864044243652 0.15599452033620265
	)");

  Matrix<double> B = Matrix<double>(R"(
	0.05808361216819946 0.8661761457749352;
 0.6011150117432088 0.7080725777960455;
 0.020584494295802447 0.9699098521619943
	)");

  Matrix<double> C = Matrix<double>(R"(
	0.8324426408004217 0.21233911067827616 0.18182496720710062;
 0.18340450985343382 0.3042422429595377 0.5247564316322378;
 0.43194501864211576 0.2912291401980419 0.6118528947223795
	)");

  Matrix<double> R = Matrix<double>(R"(
	0.5402983414818157 0.649035344064104 0.5883544808430341;
 0.1903605457037474 0.6819611623843438 0.17089398970327166;
 0.17763558461246698 0.5504679890644331 0.16636834727714633
	)");

  gemm_tt_vector_threaded(alpha, A, B, beta, C);

  ASSERT_TRUE(R.AllClose(C));
}

TEST(blas_gemm_vectorised, blas_gemm_tt_vector_threaded2)
{

  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C = _alpha * T(_A) * T(_B) + _beta * _C),
       platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>
      gemm_tt_vector_threaded;
  // Compuing _C = _alpha * T(_A) * T(_B) + _beta * _C

  double alpha = double(0), beta = double(1);

  Matrix<double> A = Matrix<double>(R"(
	0.13949386065204183 0.29214464853521815 0.3663618432936917;
 0.45606998421703593 0.7851759613930136 0.19967378215835974
	)");

  Matrix<double> B = Matrix<double>(R"(
	0.5142344384136116 0.5924145688620425;
 0.046450412719997725 0.6075448519014384;
 0.17052412368729153 0.06505159298527952
	)");

  Matrix<double> C = Matrix<double>(R"(
	0.9488855372533332 0.9656320330745594 0.8083973481164611;
 0.3046137691733707 0.09767211400638387 0.6842330265121569;
 0.4401524937396013 0.12203823484477883 0.4951769101112702
	)");

  Matrix<double> R = Matrix<double>(R"(
	0.9488855372533332 0.9656320330745594 0.8083973481164611;
 0.3046137691733707 0.09767211400638387 0.6842330265121569;
 0.4401524937396013 0.12203823484477883 0.4951769101112702
	)");

  gemm_tt_vector_threaded(alpha, A, B, beta, C);

  ASSERT_TRUE(R.AllClose(C));
}

TEST(blas_gemm_vectorised, blas_gemm_tt_vector_threaded3)
{

  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C = _alpha * T(_A) * T(_B) + _beta * _C),
       platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>
      gemm_tt_vector_threaded;
  // Compuing _C = _alpha * T(_A) * T(_B) + _beta * _C

  double alpha = double(0.6062991247118181), beta = double(0.15262325838966606);

  Matrix<double> A = Matrix<double>(R"(
	0.034388521115218396 0.9093204020787821 0.2587799816000169;
 0.662522284353982 0.31171107608941095 0.5200680211778108
	)");

  Matrix<double> B = Matrix<double>(R"(
	0.5467102793432796 0.18485445552552704;
 0.9695846277645586 0.7751328233611146;
 0.9394989415641891 0.8948273504276488
	)");

  Matrix<double> C = Matrix<double>(R"(
	0.5978999788110851 0.9218742350231168 0.0884925020519195;
 0.1959828624191452 0.045227288910538066 0.32533033076326434;
 0.388677289689482 0.2713490317738959 0.8287375091519293
	)");

  Matrix<double> R = Matrix<double>(R"(
	0.17690577753540798 0.4722755587525347 0.3925345420662068;
 0.36625961560186515 0.6879467626290013 0.736731243050713;
 0.2033866983568809 0.43795252694156794 0.5560442798378264
	)");

  gemm_tt_vector_threaded(alpha, A, B, beta, C);

  ASSERT_TRUE(R.AllClose(C));
}
