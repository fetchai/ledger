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
#include "math/linalg/blas/gemm_tt_novector.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"

using namespace fetch;
using namespace fetch::math;
using namespace fetch::math::linalg;

TEST(blas_DGEMM, blas_gemm_tt_novector1)
{

  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * T(_A) * T(_B) + _beta * _C), platform::Parallelisation::NOT_PARALLEL>
      gemm_tt_novector;
  // Compuing _C <= _alpha * T(_A) * T(_B) + _beta * _C
  using Type = double;
  Type alpha = Type(1);
  Type beta  = Type(0);

  Tensor<Type>     A_tensor = Tensor<Type>::FromString(R"(
  	0.3745401188473625 0.9507143064099162 0.7319939418114051;
 0.5986584841970366 0.15601864044243652 0.15599452033620265
  	)");
  TensorView<Type> A        = A_tensor.View();
  Tensor<Type>     B_tensor = Tensor<Type>::FromString(R"(
  	0.05808361216819946 0.8661761457749352;
 0.6011150117432088 0.7080725777960455;
 0.020584494295802447 0.9699098521619943
  	)");
  TensorView<Type> B        = B_tensor.View();
  Tensor<Type>     C_tensor = Tensor<Type>::FromString(R"(
  	0.8324426408004217 0.21233911067827616 0.18182496720710062;
 0.18340450985343382 0.3042422429595377 0.5247564316322378;
 0.43194501864211576 0.2912291401980419 0.6118528947223795
  	)");
  TensorView<Type> C        = C_tensor.View();
  gemm_tt_novector(alpha, A, B, beta, C);

  Tensor<Type>     refC_tensor = Tensor<Type>::FromString(R"(
  0.5402983414818157 0.649035344064104 0.5883544808430341;
 0.1903605457037474 0.6819611623843438 0.17089398970327166;
 0.17763558461246698 0.5504679890644331 0.16636834727714633
  )");
  TensorView<Type> refC        = refC_tensor.View();

  ASSERT_TRUE(refC_tensor.AllClose(C_tensor));
}

TEST(blas_DGEMM, blas_gemm_tt_novector2)
{

  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * T(_A) * T(_B) + _beta * _C), platform::Parallelisation::NOT_PARALLEL>
      gemm_tt_novector;
  // Compuing _C <= _alpha * T(_A) * T(_B) + _beta * _C
  using Type = double;
  Type alpha = Type(0);
  Type beta  = Type(1);

  Tensor<Type>     A_tensor = Tensor<Type>::FromString(R"(
  	0.13949386065204183 0.29214464853521815 0.3663618432936917;
 0.45606998421703593 0.7851759613930136 0.19967378215835974
  	)");
  TensorView<Type> A        = A_tensor.View();
  Tensor<Type>     B_tensor = Tensor<Type>::FromString(R"(
  	0.5142344384136116 0.5924145688620425;
 0.046450412719997725 0.6075448519014384;
 0.17052412368729153 0.06505159298527952
  	)");
  TensorView<Type> B        = B_tensor.View();
  Tensor<Type>     C_tensor = Tensor<Type>::FromString(R"(
  	0.9488855372533332 0.9656320330745594 0.8083973481164611;
 0.3046137691733707 0.09767211400638387 0.6842330265121569;
 0.4401524937396013 0.12203823484477883 0.4951769101112702
  	)");
  TensorView<Type> C        = C_tensor.View();
  gemm_tt_novector(alpha, A, B, beta, C);

  Tensor<Type>     refC_tensor = Tensor<Type>::FromString(R"(
  0.9488855372533332 0.9656320330745594 0.8083973481164611;
 0.3046137691733707 0.09767211400638387 0.6842330265121569;
 0.4401524937396013 0.12203823484477883 0.4951769101112702
  )");
  TensorView<Type> refC        = refC_tensor.View();

  ASSERT_TRUE(refC_tensor.AllClose(C_tensor));
}

TEST(blas_DGEMM, blas_gemm_tt_novector3)
{

  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * T(_A) * T(_B) + _beta * _C), platform::Parallelisation::NOT_PARALLEL>
      gemm_tt_novector;
  // Compuing _C <= _alpha * T(_A) * T(_B) + _beta * _C
  using Type = double;
  Type alpha = Type(1);
  Type beta  = Type(1);

  Tensor<Type>     A_tensor = Tensor<Type>::FromString(R"(
  	0.034388521115218396 0.9093204020787821 0.2587799816000169;
 0.662522284353982 0.31171107608941095 0.5200680211778108
  	)");
  TensorView<Type> A        = A_tensor.View();
  Tensor<Type>     B_tensor = Tensor<Type>::FromString(R"(
  	0.5467102793432796 0.18485445552552704;
 0.9695846277645586 0.7751328233611146;
 0.9394989415641891 0.8948273504276488
  	)");
  TensorView<Type> B        = B_tensor.View();
  Tensor<Type>     C_tensor = Tensor<Type>::FromString(R"(
  	0.5978999788110851 0.9218742350231168 0.0884925020519195;
 0.1959828624191452 0.045227288910538066 0.32533033076326434;
 0.388677289689482 0.2713490317738959 0.8287375091519293
  	)");
  TensorView<Type> C        = C_tensor.View();
  gemm_tt_novector(alpha, A, B, beta, C);

  Tensor<Type>     refC_tensor = Tensor<Type>::FromString(R"(
  0.7391707329439722 1.4687595852789468 0.7136435415493719;
 0.7507388547039637 1.168507858960929 1.458563482375042;
 0.6262918566095386 0.925379917501852 1.5372321173958363
  )");
  TensorView<Type> refC        = refC_tensor.View();

  ASSERT_TRUE(refC_tensor.AllClose(C_tensor));
}

TEST(blas_DGEMM, blas_gemm_tt_novector4)
{

  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * T(_A) * T(_B) + _beta * _C), platform::Parallelisation::NOT_PARALLEL>
      gemm_tt_novector;
  // Compuing _C <= _alpha * T(_A) * T(_B) + _beta * _C
  using Type = double;
  Type alpha = Type(0.9586170106890343);
  Type beta  = Type(0.9650083245746044);

  Tensor<Type>     A_tensor = Tensor<Type>::FromString(R"(
  	0.3567533266935893 0.28093450968738076 0.5426960831582485;
 0.14092422497476265 0.8021969807540397 0.07455064367977082
  	)");
  TensorView<Type> A        = A_tensor.View();
  Tensor<Type>     B_tensor = Tensor<Type>::FromString(R"(
  	0.9868869366005173 0.7722447692966574;
 0.1987156815341724 0.005522117123602399;
 0.8154614284548342 0.7068573438476171
  	)");
  TensorView<Type> B        = B_tensor.View();
  Tensor<Type>     C_tensor = Tensor<Type>::FromString(R"(
  	0.7290071680409873 0.7712703466859457 0.07404465173409036;
 0.3584657285442726 0.11586905952512971 0.8631034258755935;
 0.6232981268275579 0.3308980248526492 0.06355835028602363
  	)");
  TensorView<Type> C        = C_tensor.View();
  gemm_tt_novector(alpha, A, B, beta, C);

  Tensor<Type>     refC_tensor = Tensor<Type>::FromString(R"(
  1.1453276272187378 0.8129870385725859 0.4458242285806813;
 1.205555525292884 0.1695769552247318 1.596085831828105;
 1.1700925384172405 0.4230933780224237 0.5360840368563562
  )");
  TensorView<Type> refC        = refC_tensor.View();

  ASSERT_TRUE(refC_tensor.AllClose(C_tensor));
}
