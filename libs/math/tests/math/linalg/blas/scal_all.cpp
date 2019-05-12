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
#include "math/linalg/blas/scal_all.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"

using namespace fetch;
using namespace fetch::math;
using namespace fetch::math::linalg;

TEST(blas_A_withA, blas_scal_all1)
{

  Blas<double, Signature(_x <= _n, _alpha, _x, _m), Computes(_x <= _alpha * _x),
       platform::Parallelisation::NOT_PARALLEL>
      scal_all;
  // Compuing _x <= _alpha * _x
  using type = double;
  type alpha = type(1.634390202664271);

  int n = 20;
  int m = 1;

  Tensor<type> x = Tensor<type>::FromString(R"(
    0.3745401188473625; 0.9507143064099162; 0.7319939418114051; 0.5986584841970366; 0.15601864044243652; 0.15599452033620265; 0.05808361216819946; 0.8661761457749352; 0.6011150117432088; 0.7080725777960455; 0.020584494295802447; 0.9699098521619943; 0.8324426408004217; 0.21233911067827616; 0.18182496720710062; 0.18340450985343382; 0.3042422429595377; 0.5247564316322378; 0.43194501864211576; 0.2912291401980419
    )");

  scal_all(n, alpha, x, m);

  Tensor<type> refx = Tensor<type>::FromString(R"(
  0.6121447007488409; 1.5538381479291246; 1.1963637269061609; 0.9784415613134799; 0.2549953373721178; 0.254955915706802; 0.09493128666305643; 1.4156698064360533; 0.9824564858675187; 1.1572668839250915; 0.03364309580385809; 1.585211159841115; 1.3605360964041822; 0.34704496213501884; 0.2971729450030376; 0.29975453402889496; 0.4972505411296712; 0.8576567706447928; 0.7059667065583098; 0.4759820534700191
  )");

  ASSERT_TRUE(refx.AllClose(x));
}

TEST(blas_A_withA, blas_scal_all2)
{

  Blas<double, Signature(_x <= _n, _alpha, _x, _m), Computes(_x <= _alpha * _x),
       platform::Parallelisation::NOT_PARALLEL>
      scal_all;
  // Compuing _x <= _alpha * _x
  using type = double;
  type alpha = type(4.714168900508284);

  int n = 10;
  int m = 2;

  Tensor<type> x = Tensor<type>::FromString(R"(
    0.6118528947223795; 0.13949386065204183; 0.29214464853521815; 0.3663618432936917; 0.45606998421703593; 0.7851759613930136; 0.19967378215835974; 0.5142344384136116; 0.5924145688620425; 0.046450412719997725; 0.6075448519014384; 0.17052412368729153; 0.06505159298527952; 0.9488855372533332; 0.9656320330745594; 0.8083973481164611; 0.3046137691733707; 0.09767211400638387; 0.6842330265121569; 0.4401524937396013
    )");

  scal_all(n, alpha, x, m);

  Tensor<type> refx = Tensor<type>::FromString(R"(
  2.8843778879862105; 0.13949386065204183; 1.3772192165746484; 0.3663618432936917; 2.1499909360512546; 0.7851759613930136; 0.9412959340978053; 0.5142344384136116; 2.7927423367374637; 0.046450412719997725; 2.8640690464976717; 0.17052412368729153; 0.30666419657972754; 0.9488855372533332; 4.5521524996546745; 0.8083973481164611; 1.436000757303713; 0.09767211400638387; 3.22559005428427; 0.4401524937396013
  )");

  ASSERT_TRUE(refx.AllClose(x));
}

TEST(blas_A_withA, blas_scal_all3)
{

  Blas<double, Signature(_x <= _n, _alpha, _x, _m), Computes(_x <= _alpha * _x),
       platform::Parallelisation::NOT_PARALLEL>
      scal_all;
  // Compuing _x <= _alpha * _x
  using type = double;
  type alpha = type(-4.336654882843525);

  int n = 6;
  int m = 3;

  Tensor<type> x = Tensor<type>::FromString(R"(
    0.12203823484477883; 0.4951769101112702; 0.034388521115218396; 0.9093204020787821; 0.2587799816000169; 0.662522284353982; 0.31171107608941095; 0.5200680211778108; 0.5467102793432796; 0.18485445552552704; 0.9695846277645586; 0.7751328233611146; 0.9394989415641891; 0.8948273504276488; 0.5978999788110851; 0.9218742350231168; 0.0884925020519195; 0.1959828624191452; 0.045227288910538066; 0.32533033076326434
    )");

  scal_all(n, alpha, x, m);

  Tensor<type> refx = Tensor<type>::FromString(R"(
  -0.5292377070332148; 0.4951769101112702; 0.034388521115218396; -3.9434087617441875; 0.2587799816000169; 0.662522284353982; -1.3517833601595535; 0.5200680211778108; 0.5467102793432796; -0.8016499771701581; 0.9695846277645586; 0.7751328233611146; -4.074282672360664; 0.8948273504276488; 0.5978999788110851; -3.9978504026806387; 0.0884925020519195; 0.1959828624191452; 0.045227288910538066; 0.32533033076326434
  )");

  ASSERT_TRUE(refx.AllClose(x));
}

TEST(blas_A_withA, blas_scal_all4)
{

  Blas<double, Signature(_x <= _n, _alpha, _x, _m), Computes(_x <= _alpha * _x),
       platform::Parallelisation::NOT_PARALLEL>
      scal_all;
  // Compuing _x <= _alpha * _x
  using type = double;
  type alpha = type(6.5289314416782105);

  int n = 10;
  int m = -2;

  Tensor<type> x = Tensor<type>::FromString(R"(
    0.388677289689482; 0.2713490317738959; 0.8287375091519293; 0.3567533266935893; 0.28093450968738076; 0.5426960831582485; 0.14092422497476265; 0.8021969807540397; 0.07455064367977082; 0.9868869366005173; 0.7722447692966574; 0.1987156815341724; 0.005522117123602399; 0.8154614284548342; 0.7068573438476171; 0.7290071680409873; 0.7712703466859457; 0.07404465173409036; 0.3584657285442726; 0.11586905952512971
    )");

  scal_all(n, alpha, x, m);

  Tensor<type> refx = Tensor<type>::FromString(R"(
  0.388677289689482; 0.2713490317738959; 0.8287375091519293; 0.3567533266935893; 0.28093450968738076; 0.5426960831582485; 0.14092422497476265; 0.8021969807540397; 0.07455064367977082; 0.9868869366005173; 0.7722447692966574; 0.1987156815341724; 0.005522117123602399; 0.8154614284548342; 0.7068573438476171; 0.7290071680409873; 0.7712703466859457; 0.07404465173409036; 0.3584657285442726; 0.11586905952512971
  )");

  ASSERT_TRUE(refx.AllClose(x));
}
