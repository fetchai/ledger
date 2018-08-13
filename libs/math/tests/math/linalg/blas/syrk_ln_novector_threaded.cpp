#include <gtest/gtest.h>

#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/syrk_ln_novector.hpp"
#include "math/linalg/blas/syrk_ln_novector_threaded.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"

using namespace fetch;
using namespace fetch::math::linalg;

TEST(blas_A_withA, blas_syrk_ln_novector_threaded1)
{

  Blas<double, Signature(L(_C) <= _alpha, L(_A), _beta, L(_C)),
       Computes(_C = _alpha * _A * T(_A) + _beta * _C), platform::Parallelisation::THREADING>
      syrk_ln_novector_threaded;
  // Compuing _C = _alpha * _A * T(_A) + _beta * _C

  double alpha = double(1), beta = double(0);

  Matrix<double> A = Matrix<double>(R"(
	0.3745401188473625 0.9507143064099162;
 0.7319939418114051 0.5986584841970366;
 0.15601864044243652 0.15599452033620265
	)");

  Matrix<double> C = Matrix<double>(R"(
	0.05808361216819946 0.8661761457749352 0.6011150117432088;
 0.7080725777960455 0.020584494295802447 0.9699098521619943;
 0.8324426408004217 0.21233911067827616 0.18182496720710062
	)");

  Matrix<double> R = Matrix<double>(R"(
	1.0441379930386843 0.8433142835413905 0.20674146233889457;
 0.8433142835413905 0.8942071115496921 0.20759214270103027;
 0.20674146233889457 0.20759214270103027 0.04867610654042823
	)");

  syrk_ln_novector_threaded(alpha, A, beta, C);

  for (std::size_t i = 0; i < C.height(); ++i)
  {
    for (std::size_t j = 0; j < i; ++j)
    {
      C(j, i) = C(i, j);
    }
  }

  ASSERT_TRUE(R.AllClose(C));
}

TEST(blas_A_withA, blas_syrk_ln_novector_threaded2)
{

  Blas<double, Signature(L(_C) <= _alpha, L(_A), _beta, L(_C)),
       Computes(_C = _alpha * _A * T(_A) + _beta * _C), platform::Parallelisation::THREADING>
      syrk_ln_novector_threaded;
  // Compuing _C = _alpha * _A * T(_A) + _beta * _C

  double alpha = double(0), beta = double(1);

  Matrix<double> A = Matrix<double>(R"(
	0.18340450985343382 0.3042422429595377;
 0.5247564316322378 0.43194501864211576;
 0.2912291401980419 0.6118528947223795
	)");

  Matrix<double> C = Matrix<double>(R"(
	0.13949386065204183 0.29214464853521815 0.3663618432936917;
 0.45606998421703593 0.7851759613930136 0.19967378215835974;
 0.5142344384136116 0.5924145688620425 0.046450412719997725
	)");

  Matrix<double> R = Matrix<double>(R"(
	0.13949386065204183 0.45606998421703593 0.5142344384136116;
 0.45606998421703593 0.7851759613930136 0.5924145688620425;
 0.5142344384136116 0.5924145688620425 0.046450412719997725
	)");

  syrk_ln_novector_threaded(alpha, A, beta, C);

  for (std::size_t i = 0; i < C.height(); ++i)
  {
    for (std::size_t j = 0; j < i; ++j)
    {
      C(j, i) = C(i, j);
    }
  }

  ASSERT_TRUE(R.AllClose(C));
}

TEST(blas_A_withA, blas_syrk_ln_novector_threaded3)
{

  Blas<double, Signature(L(_C) <= _alpha, L(_A), _beta, L(_C)),
       Computes(_C = _alpha * _A * T(_A) + _beta * _C), platform::Parallelisation::THREADING>
      syrk_ln_novector_threaded;
  // Compuing _C = _alpha * _A * T(_A) + _beta * _C

  double alpha = double(0.7091699869554572), beta = double(0.7984077923795417);

  Matrix<double> A = Matrix<double>(R"(
	0.6075448519014384 0.17052412368729153;
 0.06505159298527952 0.9488855372533332;
 0.9656320330745594 0.8083973481164611
	)");

  Matrix<double> C = Matrix<double>(R"(
	0.3046137691733707 0.09767211400638387 0.6842330265121569;
 0.4401524937396013 0.12203823484477883 0.4951769101112702;
 0.034388521115218396 0.9093204020787821 0.2587799816000169
	)");

  Matrix<double> R = Matrix<double>(R"(
	0.52558985364434 0.49419811556712434 0.5412610796113211;
 0.49419811556712434 0.7389624205423104 1.3145433155462827;
 0.5412610796113211 1.3145433155462827 1.3313211553682285
	)");

  syrk_ln_novector_threaded(alpha, A, beta, C);

  for (std::size_t i = 0; i < C.height(); ++i)
  {
    for (std::size_t j = 0; j < i; ++j)
    {
      C(j, i) = C(i, j);
    }
  }

  ASSERT_TRUE(R.AllClose(C));
}
