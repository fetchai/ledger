//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/gemm_nn_novector.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor/tensor.hpp"

#include "gtest/gtest.h"

using namespace fetch;
using namespace fetch::math;
using namespace fetch::math::linalg;

TEST(blas_DGEMM, blas_gemm_nn_novector1)
{
  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * _A * _B + _beta * _C), platform::Parallelisation::NOT_PARALLEL>
      gemm_nn_novector;
  // Computing _C <= _alpha * _A * _B + _beta * _C
  using Type = double;
  auto alpha = Type(1);
  auto beta  = Type(0);

  Tensor<Type>     tensor_A = Tensor<Type>::FromString(R"(0.3745401188473625 0.9507143064099162;
                                                          0.7319939418114051 0.5986584841970366;
                                                          0.15601864044243652 0.15599452033620265)");
  TensorView<Type> A        = tensor_A.View();
  Tensor<Type>     tensor_B =
      Tensor<Type>::FromString(R"(0.05808361216819946 0.8661761457749352 0.6011150117432088;
                                  0.7080725777960455 0.020584494295802447 0.9699098521619943)");
  TensorView<Type> B = tensor_B.View();
  Tensor<Type>     tensor_C =
      Tensor<Type>::FromString(R"(0.8324426408004217 0.21233911067827616 0.18182496720710062;
                                  0.18340450985343382 0.3042422429595377 0.5247564316322378;
                                  0.43194501864211576 0.2912291401980419 0.6118528947223795)");
  TensorView<Type> C = tensor_C.View();
  gemm_nn_novector(alpha, A, B, beta, C);

  Tensor<Type> ref_tensor_C =
      Tensor<Type>::FromString(R"(0.6949293726918103 0.3439876897985273 1.14724886031757;
                                  0.46641050835051406 0.6463587734018926 1.0206573088309918;
                                  0.11951756833898089 0.1383506929615121 0.24508576903908222)");
  TensorView<Type> refC = ref_tensor_C.View();

  ASSERT_TRUE(ref_tensor_C.AllClose(tensor_C));
}

TEST(blas_DGEMM, blas_gemm_nn_novector2)
{
  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * _A * _B + _beta * _C), platform::Parallelisation::NOT_PARALLEL>
      gemm_nn_novector;
  // Computing _C <= _alpha * _A * _B + _beta * _C
  using Type = double;
  auto alpha = Type(0);
  auto beta  = Type(1);

  Tensor<Type>     tensor_A = Tensor<Type>::FromString(R"(0.13949386065204183 0.29214464853521815;
                                                          0.3663618432936917 0.45606998421703593;
                                                          0.7851759613930136 0.19967378215835974)");
  TensorView<Type> A        = tensor_A.View();
  Tensor<Type>     tensor_B =
      Tensor<Type>::FromString(R"(0.5142344384136116 0.5924145688620425 0.046450412719997725;
                                  0.6075448519014384 0.17052412368729153 0.06505159298527952)");
  TensorView<Type> B = tensor_B.View();
  Tensor<Type>     tensor_C =
      Tensor<Type>::FromString(R"(0.9488855372533332 0.9656320330745594 0.8083973481164611;
                                  0.3046137691733707 0.09767211400638387 0.6842330265121569;
                                  0.4401524937396013 0.12203823484477883 0.4951769101112702)");
  TensorView<Type> C = tensor_C.View();
  gemm_nn_novector(alpha, A, B, beta, C);

  Tensor<Type> ref_tensor_C =
      Tensor<Type>::FromString(R"(0.9488855372533332 0.9656320330745594 0.8083973481164611;
                                  0.3046137691733707 0.09767211400638387 0.6842330265121569;
                                  0.4401524937396013 0.12203823484477883 0.4951769101112702)");
  TensorView<Type> refC = ref_tensor_C.View();

  ASSERT_TRUE(ref_tensor_C.AllClose(tensor_C));
}

TEST(blas_DGEMM, blas_gemm_nn_novector3)
{
  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * _A * _B + _beta * _C), platform::Parallelisation::NOT_PARALLEL>
      gemm_nn_novector;
  // Computing _C <= _alpha * _A * _B + _beta * _C
  using Type = double;
  auto alpha = Type(1);
  auto beta  = Type(1);

  Tensor<Type>     tensor_A = Tensor<Type>::FromString(R"(0.034388521115218396 0.9093204020787821;
                                                          0.2587799816000169 0.662522284353982;
                                                          0.31171107608941095 0.5200680211778108)");
  TensorView<Type> A        = tensor_A.View();
  Tensor<Type>     tensor_B =
      Tensor<Type>::FromString(R"(0.5467102793432796 0.18485445552552704 0.9695846277645586;
                                  0.7751328233611146 0.9394989415641891 0.8948273504276488)");
  TensorView<Type> B = tensor_B.View();
  Tensor<Type>     tensor_C =
      Tensor<Type>::FromString(R"(0.5978999788110851 0.9218742350231168 0.0884925020519195;
                                  0.1959828624191452 0.045227288910538066 0.32533033076326434;
                                  0.388677289689482 0.2713490317738959 0.8287375091519293)");
  TensorView<Type> C = tensor_C.View();
  gemm_nn_novector(alpha, A, B, beta, C);

  Tensor<Type> ref_tensor_C =
      Tensor<Type>::FromString(R"(1.3215446273993787 1.7825366616659373 0.935519849578753;
                                  0.8510033072590965 0.7155029064233699 1.169082483203583;
                                  0.9622147327681025 0.8175735684636156 1.5963388662648617)");
  TensorView<Type> refC = ref_tensor_C.View();

  ASSERT_TRUE(ref_tensor_C.AllClose(tensor_C));
}

TEST(blas_DGEMM, blas_gemm_nn_novector4)
{
  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * _A * _B + _beta * _C), platform::Parallelisation::NOT_PARALLEL>
      gemm_nn_novector;
  // Computing _C <= _alpha * _A * _B + _beta * _C
  using Type = double;
  auto alpha = Type(0.03350148733328795);
  auto beta  = Type(0.5745327257561297);

  Tensor<Type>     tensor_A = Tensor<Type>::FromString(R"(0.3567533266935893 0.28093450968738076;
                                                          0.5426960831582485 0.14092422497476265;
                                                          0.8021969807540397 0.07455064367977082)");
  TensorView<Type> A        = tensor_A.View();
  Tensor<Type>     tensor_B =
      Tensor<Type>::FromString(R"(0.9868869366005173 0.7722447692966574 0.1987156815341724;
                                  0.005522117123602399 0.8154614284548342 0.7068573438476171)");
  TensorView<Type> B = tensor_B.View();
  Tensor<Type>     tensor_C =
      Tensor<Type>::FromString(R"(0.7290071680409873 0.7712703466859457 0.07404465173409036;
                                  0.3584657285442726 0.11586905952512971 0.8631034258755935;
                                  0.6232981268275579 0.3308980248526492 0.06355835028602363)");
  TensorView<Type> C = tensor_C.View();
  gemm_nn_novector(alpha, A, B, beta, C);

  Tensor<Type> ref_tensor_C =
      Tensor<Type>::FromString(R"(0.43068549076835544 0.46002464199889653 0.051568825293934265;
                                  0.2239190786691014 0.08446077897915318 0.5028312332035322;
                                  0.3846413447079871 0.2129023234674525 0.043622211662308716)");
  TensorView<Type> refC = ref_tensor_C.View();

  ASSERT_TRUE(ref_tensor_C.AllClose(tensor_C));
}

TEST(blas_DGEMM, blas_gemm_nn_novector5)
{
  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * _A * _B + _beta * _C), platform::Parallelisation::NOT_PARALLEL>
      gemm_nn_novector;
  // Computing _C <= _alpha * _A * _B + _beta * _C
  using Type = double;
  auto alpha = Type(3.2);
  auto beta  = Type(0);

  Tensor<Type>     tensor_A = Tensor<Type>::FromString(R"(0.3109823217156622;
                                                          0.32518332202674705;
                                                          0.7296061783380641;
                                                          0.6375574713552131)");
  TensorView<Type> A        = tensor_A.View();
  Tensor<Type>     tensor_B = Tensor<Type>::FromString(
      R"(0.8872127425763265 0.4722149251619493 0.1195942459383017 0.713244787222995)");
  TensorView<Type> B        = tensor_B.View();
  Tensor<Type>     tensor_C = Tensor<Type>::FromString(
      R"(0.7607850486168974 0.5612771975694962 0.770967179954561 0.49379559636439074;
         0.5227328293819941 0.42754101835854963 0.02541912674409519 0.10789142699330445;
         0.03142918568673425 0.6364104112637804 0.3143559810763267 0.5085706911647028;
         0.907566473926093 0.24929222914887494 0.41038292303562973 0.7555511385430487)");
  TensorView<Type> C = tensor_C.View();
  gemm_nn_novector(alpha, A, B, beta, C);

  Tensor<Type> ref_tensor_C = Tensor<Type>::FromString(
      R"(0.8829039313347398 0.4699215800820821 0.11901342805032629 0.7097808636230415;
         0.9232217183213793 0.49138053783927815 0.12444817340640241 0.7421929898477884;
         2.071410875148628 1.1024989660851352 0.2792214423368539 1.6652409709610083;
         1.8100771606754766 0.9634052915918195 0.24399425609298275 1.4551505375973752)");
  TensorView<Type> refC = ref_tensor_C.View();

  ASSERT_TRUE(ref_tensor_C.AllClose(tensor_C));
}

TEST(blas_DGEMM, blas_gemm_nn_novector6)
{
  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * _A * _B + _beta * _C), platform::Parallelisation::NOT_PARALLEL>
      gemm_nn_novector;
  // Computing _C <= _alpha * _A * _B + _beta * _C
  using Type = double;
  auto alpha = Type(0);
  auto beta  = Type(1.9);

  Tensor<Type>     tensor_A = Tensor<Type>::FromString(R"(0.22879816549162246 0.07697990982879299;
                                                          0.289751452913768 0.16122128725400442;
                                                          0.9296976523425731 0.808120379564417;
                                                          0.6334037565104235 0.8714605901877177;
                                                          0.8036720768991145 0.18657005888603584)");
  TensorView<Type> A        = tensor_A.View();
  Tensor<Type>     tensor_B = Tensor<Type>::FromString(
      R"(0.8925589984899778 0.5393422419156507 0.8074401551640625 0.8960912999234932 0.3180034749718639;
         0.11005192452767676 0.22793516254194168 0.4271077886262563 0.8180147659224931 0.8607305832563434)");
  TensorView<Type> B        = tensor_B.View();
  Tensor<Type>     tensor_C = Tensor<Type>::FromString(
      R"(0.006952130531190703 0.5107473025775657 0.417411003148779 0.22210781047073025 0.1198653673336828;
         0.33761517140362796 0.9429097039125192 0.32320293202075523 0.5187906217433661 0.7030189588951778;
         0.363629602379294 0.9717820827209607 0.9624472949421112 0.25178229582536416 0.49724850589238545;
         0.30087830981676966 0.2848404943774676 0.036886947354532795 0.6095643339798968 0.5026790232288615;
         0.05147875124998935 0.27864646423661144 0.9082658859666537 0.23956189066697242 0.1448948720912231)");
  TensorView<Type> C = tensor_C.View();
  gemm_nn_novector(alpha, A, B, beta, C);

  Tensor<Type> ref_tensor_C = Tensor<Type>::FromString(
      R"(0.013209048009262335 0.9704198748973748 0.79308090598268 0.4220048398943875 0.2277441979339973;
         0.6414688256668931 1.7915284374337863 0.6140855708394349 0.9857021813123955 1.3357360219008378;
         0.6908962445206586 1.8463859571698251 1.8286498603900112 0.47838636206819185 0.9447721611955323;
         0.5716687886518623 0.5411969393171885 0.0700851999736123 1.158172234561804 0.9550901441348367;
         0.09780962737497977 0.5294282820495617 1.725705183336642 0.45516759226724757 0.2753002569733239)");
  TensorView<Type> refC = ref_tensor_C.View();

  ASSERT_TRUE(ref_tensor_C.AllClose(tensor_C));
}

TEST(blas_DGEMM, blas_gemm_nn_novector7)
{
  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * _A * _B + _beta * _C), platform::Parallelisation::NOT_PARALLEL>
      gemm_nn_novector;
  // Computing _C <= _alpha * _A * _B + _beta * _C
  using Type = double;
  auto alpha = Type(1.8006387209765764);
  auto beta  = Type(-0.9940099838232666);

  Tensor<Type>     tensor_A = Tensor<Type>::FromString(R"(0.489452760277563 0.9856504541106007;
                                                          0.2420552715115004 0.6721355474058786;
                                                          0.7616196153287176 0.23763754399239967;
                                                          0.7282163486118596 0.3677831327192532)");
  TensorView<Type> A        = tensor_A.View();
  Tensor<Type>     tensor_B = Tensor<Type>::FromString(
      R"(0.6323058305935795 0.6335297107608947 0.5357746840747585 0.0902897700544083 0.835302495589238 0.32078006497173583 0.18651851039985423 0.040775141554763916 0.5908929431882418;
         0.6775643618422824 0.016587828927856152 0.512093058299281 0.22649577519793795 0.6451727904094499 0.17436642900499144 0.690937738102466 0.3867353463005374 0.9367299887367345)");
  TensorView<Type> B        = tensor_B.View();
  Tensor<Type>     tensor_C = Tensor<Type>::FromString(
      R"(0.13752094414599325 0.3410663510502585 0.11347352124058907 0.9246936182785628 0.877339353380981 0.2579416277151556 0.659984046034179 0.8172222002012158 0.5552008115994623;
         0.5296505783560065 0.24185229090045168 0.09310276780589921 0.8972157579533268 0.9004180571633305 0.6331014572732679 0.3390297910487007 0.3492095746126609 0.7259556788702394;
         0.8971102599525771 0.8870864242651173 0.7798755458576239 0.6420316461542878 0.08413996499504883 0.16162871409461377 0.8985541885270792 0.6064290596595899 0.009197051616629648;
         0.1014715428660321 0.6635017691080558 0.005061583846218687 0.16080805141749865 0.5487337893665861 0.6918951976926933 0.6519612595026005 0.22426930946055978 0.7121792213475359)");
  TensorView<Type> C = tensor_C.View();
  gemm_nn_novector(alpha, A, B, beta, C);

  Tensor<Type> ref_tensor_C = Tensor<Type>::FromString(
      R"(1.6231128659159049 0.24876394147896413 1.2682621501788753 -0.4375950365914927 1.0091439698527114 0.33578137234930294 0.7346297340753531 -0.09001273179803593 1.6314038385664893;
         0.5691531734392112 0.05579873298737356 0.7607470607478493 -0.5783664675151172 0.2498808716318266 -0.2784648324341799 0.5805193504812269 0.13870966964488435 0.6696352036513923;
         0.2653381392915227 -0.005851074102013354 0.1786825649293825 -0.4174449177502263 1.3379683819220383 0.35386907735256423 -0.3417282009053928 -0.38139363004513227 1.202035832786575;
         1.1769634792556012 0.18217653520443466 1.0366365666292074 0.10854365627982615 0.9771095007451163 -0.15165319580151637 0.0540869964976205 0.08665403266997648 0.6872421362992716)");
  TensorView<Type> refC = ref_tensor_C.View();

  ASSERT_TRUE(ref_tensor_C.AllClose(tensor_C));
}
