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
#include "math/linalg/blas/swap_all.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"

using namespace fetch;
using namespace fetch::math;
using namespace fetch::math::linalg;

TEST(blas_A_withA, blas_swap_all1)
{

  Blas<double, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
       platform::Parallelisation::NOT_PARALLEL>
      swap_all;
  // Compuing _x, _y <= _y, _x
  using type = double;

  int n = 20;
  int m = 1;
  int p = 1;

  Tensor<type> x = Tensor<type>::FromString(R"(
    0.3745401188473625; 0.9507143064099162; 0.7319939418114051; 0.5986584841970366; 0.15601864044243652; 0.15599452033620265; 0.05808361216819946; 0.8661761457749352; 0.6011150117432088; 0.7080725777960455; 0.020584494295802447; 0.9699098521619943; 0.8324426408004217; 0.21233911067827616; 0.18182496720710062; 0.18340450985343382; 0.3042422429595377; 0.5247564316322378; 0.43194501864211576; 0.2912291401980419
    )");

  Tensor<type> y = Tensor<type>::FromString(R"(
    0.6118528947223795; 0.13949386065204183; 0.29214464853521815; 0.3663618432936917; 0.45606998421703593; 0.7851759613930136; 0.19967378215835974; 0.5142344384136116; 0.5924145688620425; 0.046450412719997725; 0.6075448519014384; 0.17052412368729153; 0.06505159298527952; 0.9488855372533332; 0.9656320330745594; 0.8083973481164611; 0.3046137691733707; 0.09767211400638387; 0.6842330265121569; 0.4401524937396013
    )");

  swap_all(n, x, m, y, p);

  Tensor<type> refx = Tensor<type>::FromString(R"(
  0.6118528947223795; 0.13949386065204183; 0.29214464853521815; 0.3663618432936917; 0.45606998421703593; 0.7851759613930136; 0.19967378215835974; 0.5142344384136116; 0.5924145688620425; 0.046450412719997725; 0.6075448519014384; 0.17052412368729153; 0.06505159298527952; 0.9488855372533332; 0.9656320330745594; 0.8083973481164611; 0.3046137691733707; 0.09767211400638387; 0.6842330265121569; 0.4401524937396013
  )");

  ASSERT_TRUE(refx.AllClose(x));

  Tensor<type> refy = Tensor<type>::FromString(R"(
  0.3745401188473625; 0.9507143064099162; 0.7319939418114051; 0.5986584841970366; 0.15601864044243652; 0.15599452033620265; 0.05808361216819946; 0.8661761457749352; 0.6011150117432088; 0.7080725777960455; 0.020584494295802447; 0.9699098521619943; 0.8324426408004217; 0.21233911067827616; 0.18182496720710062; 0.18340450985343382; 0.3042422429595377; 0.5247564316322378; 0.43194501864211576; 0.2912291401980419
  )");

  ASSERT_TRUE(refy.AllClose(y));
}

TEST(blas_A_withA, blas_swap_all2)
{

  Blas<double, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
       platform::Parallelisation::NOT_PARALLEL>
      swap_all;
  // Compuing _x, _y <= _y, _x
  using type = double;

  int n = 10;
  int m = 2;
  int p = 2;

  Tensor<type> x = Tensor<type>::FromString(R"(
    0.12203823484477883; 0.4951769101112702; 0.034388521115218396; 0.9093204020787821; 0.2587799816000169; 0.662522284353982; 0.31171107608941095; 0.5200680211778108; 0.5467102793432796; 0.18485445552552704; 0.9695846277645586; 0.7751328233611146; 0.9394989415641891; 0.8948273504276488; 0.5978999788110851; 0.9218742350231168; 0.0884925020519195; 0.1959828624191452; 0.045227288910538066; 0.32533033076326434
    )");

  Tensor<type> y = Tensor<type>::FromString(R"(
    0.388677289689482; 0.2713490317738959; 0.8287375091519293; 0.3567533266935893; 0.28093450968738076; 0.5426960831582485; 0.14092422497476265; 0.8021969807540397; 0.07455064367977082; 0.9868869366005173; 0.7722447692966574; 0.1987156815341724; 0.005522117123602399; 0.8154614284548342; 0.7068573438476171; 0.7290071680409873; 0.7712703466859457; 0.07404465173409036; 0.3584657285442726; 0.11586905952512971
    )");

  swap_all(n, x, m, y, p);

  Tensor<type> refx = Tensor<type>::FromString(R"(
  0.388677289689482; 0.4951769101112702; 0.8287375091519293; 0.9093204020787821; 0.28093450968738076; 0.662522284353982; 0.14092422497476265; 0.5200680211778108; 0.07455064367977082; 0.18485445552552704; 0.7722447692966574; 0.7751328233611146; 0.005522117123602399; 0.8948273504276488; 0.7068573438476171; 0.9218742350231168; 0.7712703466859457; 0.1959828624191452; 0.3584657285442726; 0.32533033076326434
  )");

  ASSERT_TRUE(refx.AllClose(x));

  Tensor<type> refy = Tensor<type>::FromString(R"(
  0.12203823484477883; 0.2713490317738959; 0.034388521115218396; 0.3567533266935893; 0.2587799816000169; 0.5426960831582485; 0.31171107608941095; 0.8021969807540397; 0.5467102793432796; 0.9868869366005173; 0.9695846277645586; 0.1987156815341724; 0.9394989415641891; 0.8154614284548342; 0.5978999788110851; 0.7290071680409873; 0.0884925020519195; 0.07404465173409036; 0.045227288910538066; 0.11586905952512971
  )");

  ASSERT_TRUE(refy.AllClose(y));
}

TEST(blas_A_withA, blas_swap_all3)
{

  Blas<double, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
       platform::Parallelisation::NOT_PARALLEL>
      swap_all;
  // Compuing _x, _y <= _y, _x
  using type = double;

  int n = 10;
  int m = 3;
  int p = 3;

  Tensor<type> x = Tensor<type>::FromString(R"(
    0.8631034258755935; 0.6232981268275579; 0.3308980248526492; 0.06355835028602363; 0.3109823217156622; 0.32518332202674705; 0.7296061783380641; 0.6375574713552131; 0.8872127425763265; 0.4722149251619493; 0.1195942459383017; 0.713244787222995; 0.7607850486168974; 0.5612771975694962; 0.770967179954561; 0.49379559636439074; 0.5227328293819941; 0.42754101835854963; 0.02541912674409519; 0.10789142699330445; 0.03142918568673425; 0.6364104112637804; 0.3143559810763267; 0.5085706911647028; 0.907566473926093; 0.24929222914887494; 0.41038292303562973; 0.7555511385430487; 0.22879816549162246; 0.07697990982879299; 0.289751452913768; 0.16122128725400442; 0.9296976523425731; 0.808120379564417; 0.6334037565104235; 0.8714605901877177; 0.8036720768991145; 0.18657005888603584; 0.8925589984899778; 0.5393422419156507
    )");

  Tensor<type> y = Tensor<type>::FromString(R"(
    0.8074401551640625; 0.8960912999234932; 0.3180034749718639; 0.11005192452767676; 0.22793516254194168; 0.4271077886262563; 0.8180147659224931; 0.8607305832563434; 0.006952130531190703; 0.5107473025775657; 0.417411003148779; 0.22210781047073025; 0.1198653673336828; 0.33761517140362796; 0.9429097039125192; 0.32320293202075523; 0.5187906217433661; 0.7030189588951778; 0.363629602379294; 0.9717820827209607; 0.9624472949421112; 0.25178229582536416; 0.49724850589238545; 0.30087830981676966; 0.2848404943774676; 0.036886947354532795; 0.6095643339798968; 0.5026790232288615; 0.05147875124998935; 0.27864646423661144; 0.9082658859666537; 0.23956189066697242; 0.1448948720912231; 0.489452760277563; 0.9856504541106007; 0.2420552715115004; 0.6721355474058786; 0.7616196153287176; 0.23763754399239967; 0.7282163486118596
    )");

  swap_all(n, x, m, y, p);

  Tensor<type> refx = Tensor<type>::FromString(R"(
  0.8074401551640625; 0.6232981268275579; 0.3308980248526492; 0.11005192452767676; 0.3109823217156622; 0.32518332202674705; 0.8180147659224931; 0.6375574713552131; 0.8872127425763265; 0.5107473025775657; 0.1195942459383017; 0.713244787222995; 0.1198653673336828; 0.5612771975694962; 0.770967179954561; 0.32320293202075523; 0.5227328293819941; 0.42754101835854963; 0.363629602379294; 0.10789142699330445; 0.03142918568673425; 0.25178229582536416; 0.3143559810763267; 0.5085706911647028; 0.2848404943774676; 0.24929222914887494; 0.41038292303562973; 0.5026790232288615; 0.22879816549162246; 0.07697990982879299; 0.289751452913768; 0.16122128725400442; 0.9296976523425731; 0.808120379564417; 0.6334037565104235; 0.8714605901877177; 0.8036720768991145; 0.18657005888603584; 0.8925589984899778; 0.5393422419156507
  )");

  ASSERT_TRUE(refx.AllClose(x));

  Tensor<type> refy = Tensor<type>::FromString(R"(
  0.8631034258755935; 0.8960912999234932; 0.3180034749718639; 0.06355835028602363; 0.22793516254194168; 0.4271077886262563; 0.7296061783380641; 0.8607305832563434; 0.006952130531190703; 0.4722149251619493; 0.417411003148779; 0.22210781047073025; 0.7607850486168974; 0.33761517140362796; 0.9429097039125192; 0.49379559636439074; 0.5187906217433661; 0.7030189588951778; 0.02541912674409519; 0.9717820827209607; 0.9624472949421112; 0.6364104112637804; 0.49724850589238545; 0.30087830981676966; 0.907566473926093; 0.036886947354532795; 0.6095643339798968; 0.7555511385430487; 0.05147875124998935; 0.27864646423661144; 0.9082658859666537; 0.23956189066697242; 0.1448948720912231; 0.489452760277563; 0.9856504541106007; 0.2420552715115004; 0.6721355474058786; 0.7616196153287176; 0.23763754399239967; 0.7282163486118596
  )");

  ASSERT_TRUE(refy.AllClose(y));
}

TEST(blas_A_withA, blas_swap_all4)
{

  Blas<double, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
       platform::Parallelisation::NOT_PARALLEL>
      swap_all;
  // Compuing _x, _y <= _y, _x
  using type = double;

  int n = 10;
  int m = 3;
  int p = 2;

  Tensor<type> x = Tensor<type>::FromString(R"(
    0.3677831327192532; 0.6323058305935795; 0.6335297107608947; 0.5357746840747585; 0.0902897700544083; 0.835302495589238; 0.32078006497173583; 0.18651851039985423; 0.040775141554763916; 0.5908929431882418; 0.6775643618422824; 0.016587828927856152; 0.512093058299281; 0.22649577519793795; 0.6451727904094499; 0.17436642900499144; 0.690937738102466; 0.3867353463005374; 0.9367299887367345; 0.13752094414599325; 0.3410663510502585; 0.11347352124058907; 0.9246936182785628; 0.877339353380981; 0.2579416277151556; 0.659984046034179; 0.8172222002012158; 0.5552008115994623; 0.5296505783560065; 0.24185229090045168; 0.09310276780589921; 0.8972157579533268; 0.9004180571633305; 0.6331014572732679; 0.3390297910487007; 0.3492095746126609; 0.7259556788702394; 0.8971102599525771; 0.8870864242651173; 0.7798755458576239
    )");

  Tensor<type> y = Tensor<type>::FromString(R"(
    0.6420316461542878; 0.08413996499504883; 0.16162871409461377; 0.8985541885270792; 0.6064290596595899; 0.009197051616629648; 0.1014715428660321; 0.6635017691080558; 0.005061583846218687; 0.16080805141749865; 0.5487337893665861; 0.6918951976926933; 0.6519612595026005; 0.22426930946055978; 0.7121792213475359; 0.23724908749680007; 0.3253996981592677; 0.7464914051180241; 0.6496328990472147; 0.8492234104941779; 0.6576128923003434; 0.5683086033354716; 0.09367476782809248; 0.3677158030594335; 0.26520236768172545; 0.24398964337908358; 0.9730105547524456; 0.3930977246667604; 0.8920465551771133; 0.6311386259972629; 0.7948113035416484; 0.5026370931051921; 0.5769038846263591; 0.4925176938188639; 0.1952429877980445; 0.7224521152615053; 0.2807723624408558; 0.02431596643145384; 0.6454722959071678; 0.17711067940704894
    )");

  swap_all(n, x, m, y, p);

  Tensor<type> refx = Tensor<type>::FromString(R"(
  0.6420316461542878; 0.6323058305935795; 0.6335297107608947; 0.16162871409461377; 0.0902897700544083; 0.835302495589238; 0.6064290596595899; 0.18651851039985423; 0.040775141554763916; 0.1014715428660321; 0.6775643618422824; 0.016587828927856152; 0.005061583846218687; 0.22649577519793795; 0.6451727904094499; 0.5487337893665861; 0.690937738102466; 0.3867353463005374; 0.6519612595026005; 0.13752094414599325; 0.3410663510502585; 0.7121792213475359; 0.9246936182785628; 0.877339353380981; 0.3253996981592677; 0.659984046034179; 0.8172222002012158; 0.6496328990472147; 0.5296505783560065; 0.24185229090045168; 0.09310276780589921; 0.8972157579533268; 0.9004180571633305; 0.6331014572732679; 0.3390297910487007; 0.3492095746126609; 0.7259556788702394; 0.8971102599525771; 0.8870864242651173; 0.7798755458576239
  )");

  ASSERT_TRUE(refx.AllClose(x));

  Tensor<type> refy = Tensor<type>::FromString(R"(
  0.3677831327192532; 0.08413996499504883; 0.5357746840747585; 0.8985541885270792; 0.32078006497173583; 0.009197051616629648; 0.5908929431882418; 0.6635017691080558; 0.512093058299281; 0.16080805141749865; 0.17436642900499144; 0.6918951976926933; 0.9367299887367345; 0.22426930946055978; 0.11347352124058907; 0.23724908749680007; 0.2579416277151556; 0.7464914051180241; 0.5552008115994623; 0.8492234104941779; 0.6576128923003434; 0.5683086033354716; 0.09367476782809248; 0.3677158030594335; 0.26520236768172545; 0.24398964337908358; 0.9730105547524456; 0.3930977246667604; 0.8920465551771133; 0.6311386259972629; 0.7948113035416484; 0.5026370931051921; 0.5769038846263591; 0.4925176938188639; 0.1952429877980445; 0.7224521152615053; 0.2807723624408558; 0.02431596643145384; 0.6454722959071678; 0.17711067940704894
  )");

  ASSERT_TRUE(refy.AllClose(y));
}

TEST(blas_A_withA, blas_swap_all5)
{

  Blas<double, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
       platform::Parallelisation::NOT_PARALLEL>
      swap_all;
  // Compuing _x, _y <= _y, _x
  using type = double;

  int n = 20;
  int m = -1;
  int p = -1;

  Tensor<type> x = Tensor<type>::FromString(R"(
    0.9404585843529143; 0.9539285770025874; 0.9148643902204485; 0.3701587002554444; 0.015456616528867428; 0.9283185625877254; 0.42818414831731433; 0.9666548190436696; 0.9636199770892528; 0.8530094554673601; 0.2944488920695857; 0.38509772860192526; 0.8511366715168569; 0.31692200515627766; 0.1694927466860925; 0.5568012624583502; 0.936154774160781; 0.696029796674973; 0.570061170089365; 0.09717649377076854
    )");

  Tensor<type> y = Tensor<type>::FromString(R"(
    0.6150072266991697; 0.9900538501042633; 0.14008401523652403; 0.5183296523637367; 0.8773730719279554; 0.7407686177542044; 0.697015740995268; 0.7024840839871093; 0.35949115121975517; 0.29359184426449336; 0.8093611554785136; 0.8101133946791808; 0.8670723185801037; 0.9132405525564713; 0.5113423988609378; 0.5015162946871996; 0.7982951789667752; 0.6499639307777652; 0.7019668772577033; 0.795792669436101
    )");

  swap_all(n, x, m, y, p);

  Tensor<type> refx = Tensor<type>::FromString(R"(
  0.6150072266991697; 0.9900538501042633; 0.14008401523652403; 0.5183296523637367; 0.8773730719279554; 0.7407686177542044; 0.697015740995268; 0.7024840839871093; 0.35949115121975517; 0.29359184426449336; 0.8093611554785136; 0.8101133946791808; 0.8670723185801037; 0.9132405525564713; 0.5113423988609378; 0.5015162946871996; 0.7982951789667752; 0.6499639307777652; 0.7019668772577033; 0.795792669436101
  )");

  ASSERT_TRUE(refx.AllClose(x));

  Tensor<type> refy = Tensor<type>::FromString(R"(
  0.9404585843529143; 0.9539285770025874; 0.9148643902204485; 0.3701587002554444; 0.015456616528867428; 0.9283185625877254; 0.42818414831731433; 0.9666548190436696; 0.9636199770892528; 0.8530094554673601; 0.2944488920695857; 0.38509772860192526; 0.8511366715168569; 0.31692200515627766; 0.1694927466860925; 0.5568012624583502; 0.936154774160781; 0.696029796674973; 0.570061170089365; 0.09717649377076854
  )");

  ASSERT_TRUE(refy.AllClose(y));
}

TEST(blas_A_withA, blas_swap_all6)
{

  Blas<double, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
       platform::Parallelisation::NOT_PARALLEL>
      swap_all;
  // Compuing _x, _y <= _y, _x
  using type = double;

  int n = 10;
  int m = -2;
  int p = -2;

  Tensor<type> x = Tensor<type>::FromString(R"(
    0.8900053418175663; 0.3379951568515358; 0.375582952639944; 0.093981939840869; 0.578280140996174; 0.035942273796742086; 0.46559801813246016; 0.5426446347075766; 0.2865412521282844; 0.5908332605690108; 0.03050024993904943; 0.03734818874921442; 0.8226005606596583; 0.3601906414112629; 0.12706051265188478; 0.5222432600548044; 0.7699935530986108; 0.21582102749684318; 0.6228904758190003; 0.085347464993768
    )");

  Tensor<type> y = Tensor<type>::FromString(R"(
    0.0516817211686077; 0.531354631568148; 0.5406351216101065; 0.6374299014982066; 0.7260913337226615; 0.9758520794625346; 0.5163003483011953; 0.32295647294124596; 0.7951861947687037; 0.2708322512620742; 0.4389714207056361; 0.07845638134226596; 0.02535074341545751; 0.9626484146779251; 0.8359801205122058; 0.695974206093698; 0.4089529444142699; 0.17329432007084578; 0.15643704267108605; 0.25024289816459533
    )");

  swap_all(n, x, m, y, p);

  Tensor<type> refx = Tensor<type>::FromString(R"(
  0.0516817211686077; 0.3379951568515358; 0.5406351216101065; 0.093981939840869; 0.7260913337226615; 0.035942273796742086; 0.5163003483011953; 0.5426446347075766; 0.7951861947687037; 0.5908332605690108; 0.4389714207056361; 0.03734818874921442; 0.02535074341545751; 0.3601906414112629; 0.8359801205122058; 0.5222432600548044; 0.4089529444142699; 0.21582102749684318; 0.15643704267108605; 0.085347464993768
  )");

  ASSERT_TRUE(refx.AllClose(x));

  Tensor<type> refy = Tensor<type>::FromString(R"(
  0.8900053418175663; 0.531354631568148; 0.375582952639944; 0.6374299014982066; 0.578280140996174; 0.9758520794625346; 0.46559801813246016; 0.32295647294124596; 0.2865412521282844; 0.2708322512620742; 0.03050024993904943; 0.07845638134226596; 0.8226005606596583; 0.9626484146779251; 0.12706051265188478; 0.695974206093698; 0.7699935530986108; 0.17329432007084578; 0.6228904758190003; 0.25024289816459533
  )");

  ASSERT_TRUE(refy.AllClose(y));
}

TEST(blas_A_withA, blas_swap_all7)
{

  Blas<double, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
       platform::Parallelisation::NOT_PARALLEL>
      swap_all;
  // Compuing _x, _y <= _y, _x
  using type = double;

  int n = 5;
  int m = -2;
  int p = -3;

  Tensor<type> x = Tensor<type>::FromString(R"(
    0.5492266647061205; 0.7145959227000623; 0.6601973767177313; 0.27993389694594284; 0.9548652806631941; 0.7378969166957685; 0.5543540525114007; 0.6117207462343522; 0.4196000624277899; 0.24773098950115746; 0.3559726786512616; 0.7578461104643691; 0.014393488629755868; 0.11607264050691624; 0.04600264202175275; 0.040728802318970136; 0.8554605840110072; 0.7036578593800237; 0.4741738290873252; 0.09783416065100148
    )");

  Tensor<type> y = Tensor<type>::FromString(R"(
    0.49161587511683236; 0.4734717707805657; 0.17320186991001518; 0.43385164923797304; 0.39850473439737344; 0.6158500980522165; 0.6350936508676438; 0.04530400977204452; 0.3746126146264712; 0.6258599157142364; 0.5031362585800877; 0.8564898411883223; 0.658693631618945; 0.1629344270814297; 0.07056874740042984; 0.6424192782063156; 0.026511310541621813; 0.5857755812734633; 0.9402302414249576; 0.575474177875879; 0.3881699262065219; 0.6432882184423532; 0.45825289049151663; 0.5456167893159349; 0.9414648087765252; 0.38610263780077425; 0.9611905638239142; 0.9053506419560637; 0.19579113478929644; 0.06936130087516545
    )");

  swap_all(n, x, m, y, p);

  Tensor<type> refx = Tensor<type>::FromString(R"(
  0.49161587511683236; 0.7145959227000623; 0.43385164923797304; 0.27993389694594284; 0.6350936508676438; 0.7378969166957685; 0.6258599157142364; 0.6117207462343522; 0.658693631618945; 0.24773098950115746; 0.3559726786512616; 0.7578461104643691; 0.014393488629755868; 0.11607264050691624; 0.04600264202175275; 0.040728802318970136; 0.8554605840110072; 0.7036578593800237; 0.4741738290873252; 0.09783416065100148
  )");

  ASSERT_TRUE(refx.AllClose(x));

  Tensor<type> refy = Tensor<type>::FromString(R"(
  0.5492266647061205; 0.4734717707805657; 0.17320186991001518; 0.6601973767177313; 0.39850473439737344; 0.6158500980522165; 0.9548652806631941; 0.04530400977204452; 0.3746126146264712; 0.5543540525114007; 0.5031362585800877; 0.8564898411883223; 0.4196000624277899; 0.1629344270814297; 0.07056874740042984; 0.6424192782063156; 0.026511310541621813; 0.5857755812734633; 0.9402302414249576; 0.575474177875879; 0.3881699262065219; 0.6432882184423532; 0.45825289049151663; 0.5456167893159349; 0.9414648087765252; 0.38610263780077425; 0.9611905638239142; 0.9053506419560637; 0.19579113478929644; 0.06936130087516545
  )");

  ASSERT_TRUE(refy.AllClose(y));
}
