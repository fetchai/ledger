//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <gtest/gtest.h>

#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/gemm_nt_novector.hpp"
#include "math/linalg/blas/gemm_nt_novector_threaded.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"

using namespace fetch;
using namespace fetch::math::linalg;

TEST(blas_DGEMM, blas_gemm_nt_novector_threaded1)
{

  Blas<double,
       Matrix<
           double, fetch::memory::SharedArray<double>,
           fetch::math::RectangularArray<double, fetch::memory::SharedArray<double>, true, false>>,
       Signature(_C <= _alpha, _A, _B, _beta, _C), Computes(_C = _alpha * _A * T(_B) + _beta * _C),
       platform::Parallelisation::THREADING>
      gemm_nt_novector_threaded;
  // Compuing _C = _alpha * _A * T(_B) + _beta * _C
  using type = double;

  type alpha = type(1);
  type beta  = type(0);

  Matrix<type> A = Matrix<type>(R"(
	0.3745401188473625 0.9507143064099162;
 0.7319939418114051 0.5986584841970366;
 0.15601864044243652 0.15599452033620265
	)");

  Matrix<type> B = Matrix<type>(R"(
	0.05808361216819946 0.8661761457749352;
 0.6011150117432088 0.7080725777960455;
 0.020584494295802447 0.9699098521619943
	)");

  Matrix<type> C = Matrix<type>(R"(
	0.8324426408004217 0.21233911067827616 0.18182496720710062;
 0.18340450985343382 0.3042422429595377 0.5247564316322378;
 0.43194501864211576 0.2912291401980419 0.6118528947223795
	)");

  Matrix<type> R = Matrix<type>(R"(
	0.8452406966637934 0.898316417626484 0.9298168913182975;
 0.5610605507028994 0.8639062030527893 0.5957124870228502;
 0.14418085858929 0.20424058901822734 0.15451218697159372
	)");

  gemm_nt_novector_threaded(alpha, A, B, beta, C);

  ASSERT_TRUE(R.AllClose(C));
}

TEST(blas_DGEMM, blas_gemm_nt_novector_threaded2)
{

  Blas<double,
       Matrix<
           double, fetch::memory::SharedArray<double>,
           fetch::math::RectangularArray<double, fetch::memory::SharedArray<double>, true, false>>,
       Signature(_C <= _alpha, _A, _B, _beta, _C), Computes(_C = _alpha * _A * T(_B) + _beta * _C),
       platform::Parallelisation::THREADING>
      gemm_nt_novector_threaded;
  // Compuing _C = _alpha * _A * T(_B) + _beta * _C
  using type = double;

  type alpha = type(0);
  type beta  = type(1);

  Matrix<type> A = Matrix<type>(R"(
	0.13949386065204183 0.29214464853521815;
 0.3663618432936917 0.45606998421703593;
 0.7851759613930136 0.19967378215835974
	)");

  Matrix<type> B = Matrix<type>(R"(
	0.5142344384136116 0.5924145688620425;
 0.046450412719997725 0.6075448519014384;
 0.17052412368729153 0.06505159298527952
	)");

  Matrix<type> C = Matrix<type>(R"(
	0.9488855372533332 0.9656320330745594 0.8083973481164611;
 0.3046137691733707 0.09767211400638387 0.6842330265121569;
 0.4401524937396013 0.12203823484477883 0.4951769101112702
	)");

  Matrix<type> R = Matrix<type>(R"(
	0.9488855372533332 0.9656320330745594 0.8083973481164611;
 0.3046137691733707 0.09767211400638387 0.6842330265121569;
 0.4401524937396013 0.12203823484477883 0.4951769101112702
	)");

  gemm_nt_novector_threaded(alpha, A, B, beta, C);

  ASSERT_TRUE(R.AllClose(C));
}

TEST(blas_DGEMM, blas_gemm_nt_novector_threaded3)
{

  Blas<double,
       Matrix<
           double, fetch::memory::SharedArray<double>,
           fetch::math::RectangularArray<double, fetch::memory::SharedArray<double>, true, false>>,
       Signature(_C <= _alpha, _A, _B, _beta, _C), Computes(_C = _alpha * _A * T(_B) + _beta * _C),
       platform::Parallelisation::THREADING>
      gemm_nt_novector_threaded;
  // Compuing _C = _alpha * _A * T(_B) + _beta * _C
  using type = double;

  type alpha = type(1);
  type beta  = type(1);

  Matrix<type> A = Matrix<type>(R"(
	0.034388521115218396 0.9093204020787821;
 0.2587799816000169 0.662522284353982;
 0.31171107608941095 0.5200680211778108
	)");

  Matrix<type> B = Matrix<type>(R"(
	0.5467102793432796 0.18485445552552704;
 0.9695846277645586 0.7751328233611146;
 0.9394989415641891 0.8948273504276488
	)");

  Matrix<type> C = Matrix<type>(R"(
	0.5978999788110851 0.9218742350231168 0.0884925020519195;
 0.1959828624191452 0.045227288910538066 0.32533033076326434;
 0.388677289689482 0.2713490317738959 0.8287375091519293
	)");

  Matrix<type> R = Matrix<type>(R"(
	0.7847924646207151 1.6600609070711796 0.9344852473235857;
 0.45993073459592293 0.809679149854067 1.1612969098822274;
 0.6552298300637807 0.9767010930495218 1.5869608246444562
	)");

  gemm_nt_novector_threaded(alpha, A, B, beta, C);

  ASSERT_TRUE(R.AllClose(C));
}

TEST(blas_DGEMM, blas_gemm_nt_novector_threaded4)
{

  Blas<double,
       Matrix<
           double, fetch::memory::SharedArray<double>,
           fetch::math::RectangularArray<double, fetch::memory::SharedArray<double>, true, false>>,
       Signature(_C <= _alpha, _A, _B, _beta, _C), Computes(_C = _alpha * _A * T(_B) + _beta * _C),
       platform::Parallelisation::THREADING>
      gemm_nt_novector_threaded;
  // Compuing _C = _alpha * _A * T(_B) + _beta * _C
  using type = double;

  type alpha = type(0.41214434296716984);
  type beta  = type(0.4029988924556921);

  Matrix<type> A = Matrix<type>(R"(
	0.3567533266935893 0.28093450968738076;
 0.5426960831582485 0.14092422497476265;
 0.8021969807540397 0.07455064367977082
	)");

  Matrix<type> B = Matrix<type>(R"(
	0.9868869366005173 0.7722447692966574;
 0.1987156815341724 0.005522117123602399;
 0.8154614284548342 0.7068573438476171
	)");

  Matrix<type> C = Matrix<type>(R"(
	0.7290071680409873 0.7712703466859457 0.07404465173409036;
 0.3584657285442726 0.11586905952512971 0.8631034258755935;
 0.6232981268275579 0.3308980248526492 0.06355835028602363
	)");

  Matrix<type> R = Matrix<type>(R"(
	0.5283096822972457 0.34067841174892893 0.2315842382740552;
 0.41005026759614926 0.09146239516044333 0.5712783330105866;
 0.6012016529037635 0.1992207749616569 0.31694120937132453
	)");

  gemm_nt_novector_threaded(alpha, A, B, beta, C);

  ASSERT_TRUE(R.AllClose(C));
}

TEST(blas_DGEMM, blas_gemm_nt_novector_threaded5)
{

  Blas<double,
       Matrix<
           double, fetch::memory::SharedArray<double>,
           fetch::math::RectangularArray<double, fetch::memory::SharedArray<double>, true, false>>,
       Signature(_C <= _alpha, _A, _B, _beta, _C), Computes(_C = _alpha * _A * T(_B) + _beta * _C),
       platform::Parallelisation::THREADING>
      gemm_nt_novector_threaded;
  // Compuing _C = _alpha * _A * T(_B) + _beta * _C
  using type = double;

  type alpha = type(0.823943292671352);
  type beta  = type(0.49022718625829853);

  Matrix<type> A = Matrix<type>(R"(
	0.3109823217156622;
 0.32518332202674705;
 0.7296061783380641;
 0.6375574713552131
	)");

  Matrix<type> B = Matrix<type>(R"(
	0.8872127425763265;
 0.4722149251619493;
 0.1195942459383017;
 0.713244787222995
	)");

  Matrix<type> C = Matrix<type>(R"(
	0.7607850486168974 0.5612771975694962 0.770967179954561 0.49379559636439074;
 0.5227328293819941 0.42754101835854963 0.02541912674409519 0.10789142699330445;
 0.03142918568673425 0.6364104112637804 0.3143559810763267 0.5085706911647028;
 0.907566473926093 0.24929222914887494 0.41038292303562973 0.7555511385430487
	)");

  Matrix<type> R = Matrix<type>(R"(
	0.6002896300734778 0.39614982064736076 0.40859292000783565 0.42482802012016774;
 0.49397107613134317 0.33611401115926764 0.04450434628196541 0.24399285313031469;
 0.5487590340609746 0.5958596315577507 0.2260004213938494 0.6780852191633767;
 0.9109765512789554 0.37026961809270187 0.2640050627601343 0.7450671853723593
	)");

  gemm_nt_novector_threaded(alpha, A, B, beta, C);

  ASSERT_TRUE(R.AllClose(C));
}

TEST(blas_DGEMM, blas_gemm_nt_novector_threaded6)
{

  Blas<double,
       Matrix<
           double, fetch::memory::SharedArray<double>,
           fetch::math::RectangularArray<double, fetch::memory::SharedArray<double>, true, false>>,
       Signature(_C <= _alpha, _A, _B, _beta, _C), Computes(_C = _alpha * _A * T(_B) + _beta * _C),
       platform::Parallelisation::THREADING>
      gemm_nt_novector_threaded;
  // Compuing _C = _alpha * _A * T(_B) + _beta * _C
  using type = double;

  type alpha = type(0.2573698282905107);
  type beta  = type(0.41564119803827115);

  Matrix<type> A = Matrix<type>(R"(
	0.22879816549162246 0.07697990982879299 0.289751452913768 0.16122128725400442;
 0.9296976523425731 0.808120379564417 0.6334037565104235 0.8714605901877177;
 0.8036720768991145 0.18657005888603584 0.8925589984899778 0.5393422419156507
	)");

  Matrix<type> B = Matrix<type>(R"(
	0.8074401551640625 0.8960912999234932 0.3180034749718639 0.11005192452767676;
 0.22793516254194168 0.4271077886262563 0.8180147659224931 0.8607305832563434;
 0.006952130531190703 0.5107473025775657 0.417411003148779 0.22210781047073025;
 0.1198653673336828 0.33761517140362796 0.9429097039125192 0.32320293202075523;
 0.5187906217433661 0.7030189588951778 0.363629602379294 0.9717820827209607
	)");

  Matrix<type> C = Matrix<type>(R"(
	0.9624472949421112 0.25178229582536416 0.49724850589238545 0.30087830981676966 0.2848404943774676;
 0.036886947354532795 0.6095643339798968 0.5026790232288615 0.05147875124998935 0.27864646423661144;
 0.9082658859666537 0.23956189066697242 0.1448948720912231 0.489452760277563 0.9856504541106007
	)");

  Matrix<type> R = Matrix<type>(R"(
	0.49361409833930947 0.22325198284632836 0.25754916567356256 0.22253140790527243 0.230308931001989;
 0.4714310579657737 0.723135061840277 0.4346877509687437 0.3464994153974834 0.6634063946163445;
 0.6758798714588393 0.4746176559719371 0.21290467167272237 0.5059082715636218 0.7691669490193235
	)");

  gemm_nt_novector_threaded(alpha, A, B, beta, C);

  ASSERT_TRUE(R.AllClose(C));
}
