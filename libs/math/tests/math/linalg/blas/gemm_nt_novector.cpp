#include <gtest/gtest.h>

#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/gemm_nt_novector.hpp"


using namespace fetch;
using namespace fetch::math;
using namespace fetch::math::linalg;

TEST(blas_DGEMM, blas_gemm_nt_novector1) {

	Blas< double, 
        Signature( _C <= _alpha, _A, _B, _beta, _C ), 
        Computes( _C <= _alpha * _A * T(_B) + _beta * _C ), 
        platform::Parallelisation::NOT_PARALLEL> gemm_nt_novector;
	// Compuing _C <= _alpha * _A * T(_B) + _beta * _C
  using type = double;type alpha = type(1);
  type beta = type(0);
  
  
  


  Tensor< type > A = Tensor< type >::FromString(R"(
  	0.3745401188473625 0.9507143064099162;
 0.7319939418114051 0.5986584841970366;
 0.15601864044243652 0.15599452033620265
  	)");

  Tensor< type > B = Tensor< type >::FromString(R"(
  	0.05808361216819946 0.8661761457749352;
 0.6011150117432088 0.7080725777960455;
 0.020584494295802447 0.9699098521619943
  	)");

  Tensor< type > C = Tensor< type >::FromString(R"(
  	0.8324426408004217 0.21233911067827616 0.18182496720710062;
 0.18340450985343382 0.3042422429595377 0.5247564316322378;
 0.43194501864211576 0.2912291401980419 0.6118528947223795
  	)");

  gemm_nt_novector(alpha, A, B, beta, C);

  Tensor< type > refC = Tensor< type >::FromString(R"(
  0.8452406966637934 0.898316417626484 0.9298168913182975;
 0.5610605507028993 0.8639062030527893 0.5957124870228502;
 0.14418085858929 0.20424058901822734 0.15451218697159372
  )");


  ASSERT_TRUE( refC.AllClose(C) );

 
}

TEST(blas_DGEMM, blas_gemm_nt_novector2) {

	Blas< double, 
        Signature( _C <= _alpha, _A, _B, _beta, _C ), 
        Computes( _C <= _alpha * _A * T(_B) + _beta * _C ), 
        platform::Parallelisation::NOT_PARALLEL> gemm_nt_novector;
	// Compuing _C <= _alpha * _A * T(_B) + _beta * _C
  using type = double;type alpha = type(0);
  type beta = type(1);
  
  
  


  Tensor< type > A = Tensor< type >::FromString(R"(
  	0.13949386065204183 0.29214464853521815;
 0.3663618432936917 0.45606998421703593;
 0.7851759613930136 0.19967378215835974
  	)");

  Tensor< type > B = Tensor< type >::FromString(R"(
  	0.5142344384136116 0.5924145688620425;
 0.046450412719997725 0.6075448519014384;
 0.17052412368729153 0.06505159298527952
  	)");

  Tensor< type > C = Tensor< type >::FromString(R"(
  	0.9488855372533332 0.9656320330745594 0.8083973481164611;
 0.3046137691733707 0.09767211400638387 0.6842330265121569;
 0.4401524937396013 0.12203823484477883 0.4951769101112702
  	)");

  gemm_nt_novector(alpha, A, B, beta, C);

  Tensor< type > refC = Tensor< type >::FromString(R"(
  0.9488855372533332 0.9656320330745594 0.8083973481164611;
 0.3046137691733707 0.09767211400638387 0.6842330265121569;
 0.4401524937396013 0.12203823484477883 0.4951769101112702
  )");


  ASSERT_TRUE( refC.AllClose(C) );

 
}

TEST(blas_DGEMM, blas_gemm_nt_novector3) {

	Blas< double, 
        Signature( _C <= _alpha, _A, _B, _beta, _C ), 
        Computes( _C <= _alpha * _A * T(_B) + _beta * _C ), 
        platform::Parallelisation::NOT_PARALLEL> gemm_nt_novector;
	// Compuing _C <= _alpha * _A * T(_B) + _beta * _C
  using type = double;type alpha = type(1);
  type beta = type(1);
  
  
  


  Tensor< type > A = Tensor< type >::FromString(R"(
  	0.034388521115218396 0.9093204020787821;
 0.2587799816000169 0.662522284353982;
 0.31171107608941095 0.5200680211778108
  	)");

  Tensor< type > B = Tensor< type >::FromString(R"(
  	0.5467102793432796 0.18485445552552704;
 0.9695846277645586 0.7751328233611146;
 0.9394989415641891 0.8948273504276488
  	)");

  Tensor< type > C = Tensor< type >::FromString(R"(
  	0.5978999788110851 0.9218742350231168 0.0884925020519195;
 0.1959828624191452 0.045227288910538066 0.32533033076326434;
 0.388677289689482 0.2713490317738959 0.8287375091519293
  	)");

  gemm_nt_novector(alpha, A, B, beta, C);

  Tensor< type > refC = Tensor< type >::FromString(R"(
  0.7847924646207151 1.6600609070711798 0.9344852473235858;
 0.459930734595923 0.809679149854067 1.1612969098822274;
 0.6552298300637807 0.9767010930495218 1.5869608246444562
  )");


  ASSERT_TRUE( refC.AllClose(C) );

 
}

TEST(blas_DGEMM, blas_gemm_nt_novector4) {

	Blas< double, 
        Signature( _C <= _alpha, _A, _B, _beta, _C ), 
        Computes( _C <= _alpha * _A * T(_B) + _beta * _C ), 
        platform::Parallelisation::NOT_PARALLEL> gemm_nt_novector;
	// Compuing _C <= _alpha * _A * T(_B) + _beta * _C
  using type = double;type alpha = type(0.1539397300265879);
  type beta = type(0.7903363833218108);
  
  
  


  Tensor< type > A = Tensor< type >::FromString(R"(
  	0.3567533266935893 0.28093450968738076;
 0.5426960831582485 0.14092422497476265;
 0.8021969807540397 0.07455064367977082
  	)");

  Tensor< type > B = Tensor< type >::FromString(R"(
  	0.9868869366005173 0.7722447692966574;
 0.1987156815341724 0.005522117123602399;
 0.8154614284548342 0.7068573438476171
  	)");

  Tensor< type > C = Tensor< type >::FromString(R"(
  	0.7290071680409873 0.7712703466859457 0.07404465173409036;
 0.3584657285442726 0.11586905952512971 0.8631034258755935;
 0.6232981268275579 0.3308980248526492 0.06355835028602363
  	)");

  gemm_nt_novector(alpha, A, B, beta, C);

  Tensor< type > refC = Tensor< type >::FromString(R"(
  0.6637565055711071 0.6207150005680858 0.1338735567526186;
 0.38250845028435554 0.10829653189845169 0.7656021651742524;
 0.6233483591590073 0.28612351857492907 0.1590459087537381
  )");


  ASSERT_TRUE( refC.AllClose(C) );

 
}

TEST(blas_DGEMM, blas_gemm_nt_novector5) {

	Blas< double, 
        Signature( _C <= _alpha, _A, _B, _beta, _C ), 
        Computes( _C <= _alpha * _A * T(_B) + _beta * _C ), 
        platform::Parallelisation::NOT_PARALLEL> gemm_nt_novector;
	// Compuing _C <= _alpha * _A * T(_B) + _beta * _C
  using type = double;type alpha = type(0.9119087237497633);
  type beta = type(0.6649377566530129);
  
  
  


  Tensor< type > A = Tensor< type >::FromString(R"(
  	0.3109823217156622;
 0.32518332202674705;
 0.7296061783380641;
 0.6375574713552131
  	)");

  Tensor< type > B = Tensor< type >::FromString(R"(
  	0.8872127425763265;
 0.4722149251619493;
 0.1195942459383017;
 0.713244787222995
  	)");

  Tensor< type > C = Tensor< type >::FromString(R"(
  	0.7607850486168974 0.5612771975694962 0.770967179954561 0.49379559636439074;
 0.5227328293819941 0.42754101835854963 0.02541912674409519 0.10789142699330445;
 0.03142918568673425 0.6364104112637804 0.3143559810763267 0.5085706911647028;
 0.907566473926093 0.24929222914887494 0.41038292303562973 0.7555511385430487
  	)");

  gemm_nt_novector(alpha, A, B, beta, C);

  Tensor< type > refC = Tensor< type >::FromString(R"(
  0.7574771401528204 0.5071286469733269 0.5465606193678534 0.5306106365568286;
 0.6106766508020554 0.42431760285463127 0.05236631679580402 0.283244915348307;
 0.6111914670733948 0.7373540690195248 0.28859730744890233 0.8127140321590516;
 1.1192955756484544 0.44030684369792694 0.3424105035482754 0.9170708758507584
  )");


  ASSERT_TRUE( refC.AllClose(C) );

 
}

TEST(blas_DGEMM, blas_gemm_nt_novector6) {

	Blas< double, 
        Signature( _C <= _alpha, _A, _B, _beta, _C ), 
        Computes( _C <= _alpha * _A * T(_B) + _beta * _C ), 
        platform::Parallelisation::NOT_PARALLEL> gemm_nt_novector;
	// Compuing _C <= _alpha * _A * T(_B) + _beta * _C
  using type = double;type alpha = type(0.844158755686547);
  type beta = type(0.2016749342501909);
  
  
  


  Tensor< type > A = Tensor< type >::FromString(R"(
  	0.22879816549162246 0.07697990982879299 0.289751452913768 0.16122128725400442;
 0.9296976523425731 0.808120379564417 0.6334037565104235 0.8714605901877177;
 0.8036720768991145 0.18657005888603584 0.8925589984899778 0.5393422419156507
  	)");

  Tensor< type > B = Tensor< type >::FromString(R"(
  	0.8074401551640625 0.8960912999234932 0.3180034749718639 0.11005192452767676;
 0.22793516254194168 0.4271077886262563 0.8180147659224931 0.8607305832563434;
 0.006952130531190703 0.5107473025775657 0.417411003148779 0.22210781047073025;
 0.1198653673336828 0.33761517140362796 0.9429097039125192 0.32320293202075523;
 0.5187906217433661 0.7030189588951778 0.363629602379294 0.9717820827209607
  	)");

  Tensor< type > C = Tensor< type >::FromString(R"(
  	0.9624472949421112 0.25178229582536416 0.49724850589238545 0.30087830981676966 0.2848404943774676;
 0.036886947354532795 0.6095643339798968 0.5026790232288615 0.05147875124998935 0.27864646423661144;
 0.9082658859666537 0.23956189066697242 0.1448948720912231 0.489452760277563 0.9856504541106007
  	)");

  gemm_nt_novector(alpha, A, B, beta, C);

  Tensor< type > refC = Tensor< type >::FromString(R"(
  0.5010431355396052 0.43978250881991154 0.2671405560561146 0.3803888811866157 0.42452833409897855;
 1.503419643702348 1.6637698184415766 0.8418371698348108 1.0767008740782473 1.852258999956144;
 1.1618021604103526 1.2784432540745039 0.5300048822786516 1.0908002203755738 1.377888286944805
  )");


  ASSERT_TRUE( refC.AllClose(C) );

 
}
