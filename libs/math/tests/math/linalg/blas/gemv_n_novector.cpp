#include <gtest/gtest.h>

#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
#include "math/linalg/blas/base.hpp"
#include"math/linalg/blas/gemv_n.hpp"


using namespace fetch;
using namespace fetch::math;
using namespace fetch::math::linalg;

TEST(blas_gemv, blas_gemv_n_novector1) {

	Blas< double, 
        Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), 
        Computes( _y = _alpha * _A * _x + _beta * _y ), 
        platform::Parallelisation::NOT_PARALLEL> gemv_n_novector;
	// Compuing _y = _alpha * _A * _x + _beta * _y
  using type = double;type alpha = type(1);
  type beta = type(1);
  int n = 1;
  int m = 1;
  


  Tensor< type > A = Tensor< type >::FromString(R"(
  	0.3745401188473625 0.9507143064099162;
 0.7319939418114051 0.5986584841970366;
 0.15601864044243652 0.15599452033620265
  	)");

  Tensor< type > x = Tensor< type >::FromString(R"(
    0.05808361216819946; 0.8661761457749352
    )");

  Tensor< type > y = Tensor< type >::FromString(R"(
    0.6011150117432088; 0.7080725777960455; 0.020584494295802447
    )");

  gemv_n_novector(alpha, A, x, n, beta, y, m);

  Tensor< type > refy = Tensor< type >::FromString(R"(
  1.4463557084070022; 1.2691331284989449; 0.16476535288509245
  )");



  ASSERT_TRUE( refy.AllClose(y) );
 
}

TEST(blas_gemv, blas_gemv_n_novector2) {

	Blas< double, 
        Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), 
        Computes( _y = _alpha * _A * _x + _beta * _y ), 
        platform::Parallelisation::NOT_PARALLEL> gemv_n_novector;
	// Compuing _y = _alpha * _A * _x + _beta * _y
  using type = double;type alpha = type(0);
  type beta = type(1);
  int n = 1;
  int m = 1;
  


  Tensor< type > A = Tensor< type >::FromString(R"(
  	0.9699098521619943 0.8324426408004217;
 0.21233911067827616 0.18182496720710062;
 0.18340450985343382 0.3042422429595377
  	)");

  Tensor< type > x = Tensor< type >::FromString(R"(
    0.5247564316322378; 0.43194501864211576
    )");

  Tensor< type > y = Tensor< type >::FromString(R"(
    0.2912291401980419; 0.6118528947223795; 0.13949386065204183
    )");

  gemv_n_novector(alpha, A, x, n, beta, y, m);

  Tensor< type > refy = Tensor< type >::FromString(R"(
  0.2912291401980419; 0.6118528947223795; 0.13949386065204183
  )");



  ASSERT_TRUE( refy.AllClose(y) );
 
}

TEST(blas_gemv, blas_gemv_n_novector3) {

	Blas< double, 
        Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), 
        Computes( _y = _alpha * _A * _x + _beta * _y ), 
        platform::Parallelisation::NOT_PARALLEL> gemv_n_novector;
	// Compuing _y = _alpha * _A * _x + _beta * _y
  using type = double;type alpha = type(0.01181810083989776);
  type beta = type(0.8416159123744975);
  int n = 1;
  int m = 1;
  


  Tensor< type > A = Tensor< type >::FromString(R"(
  	0.29214464853521815 0.3663618432936917 0.45606998421703593 0.7851759613930136;
 0.19967378215835974 0.5142344384136116 0.5924145688620425 0.046450412719997725;
 0.6075448519014384 0.17052412368729153 0.06505159298527952 0.9488855372533332
  	)");

  Tensor< type > x = Tensor< type >::FromString(R"(
    0.9656320330745594; 0.8083973481164611; 0.3046137691733707; 0.09767211400638387
    )");

  Tensor< type > y = Tensor< type >::FromString(R"(
    0.6842330265121569; 0.4401524937396013; 0.12203823484477883
    )");

  gemv_n_novector(alpha, A, x, n, beta, y, m);

  Tensor< type > refy = Tensor< type >::FromString(R"(
  0.5852436178366309; 0.3798171439059555; 0.11260120415981675
  )");



  ASSERT_TRUE( refy.AllClose(y) );
 
}

TEST(blas_gemv, blas_gemv_n_novector4) {

	Blas< double, 
        Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), 
        Computes( _y = _alpha * _A * _x + _beta * _y ), 
        platform::Parallelisation::NOT_PARALLEL> gemv_n_novector;
	// Compuing _y = _alpha * _A * _x + _beta * _y
  using type = double;type alpha = type(0);
  type beta = type(1);
  int n = 2;
  int m = 3;
  


  Tensor< type > A = Tensor< type >::FromString(R"(
  	0.4951769101112702 0.034388521115218396 0.9093204020787821 0.2587799816000169;
 0.662522284353982 0.31171107608941095 0.5200680211778108 0.5467102793432796;
 0.18485445552552704 0.9695846277645586 0.7751328233611146 0.9394989415641891
  	)");

  Tensor< type > x = Tensor< type >::FromString(R"(
    0.8948273504276488; 0.5978999788110851; 0.9218742350231168; 0.0884925020519195; 0.1959828624191452; 0.045227288910538066; 0.32533033076326434; 0.388677289689482
    )");

  Tensor< type > y = Tensor< type >::FromString(R"(
    0.2713490317738959; 0.8287375091519293; 0.3567533266935893; 0.28093450968738076; 0.5426960831582485; 0.14092422497476265; 0.8021969807540397; 0.07455064367977082; 0.9868869366005173
    )");

  gemv_n_novector(alpha, A, x, n, beta, y, m);

  Tensor< type > refy = Tensor< type >::FromString(R"(
  0.2713490317738959; 0.8287375091519293; 0.3567533266935893; 0.28093450968738076; 0.5426960831582485; 0.14092422497476265; 0.8021969807540397; 0.07455064367977082; 0.9868869366005173
  )");



  ASSERT_TRUE( refy.AllClose(y) );
 
}

TEST(blas_gemv, blas_gemv_n_novector5) {

	Blas< double, 
        Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), 
        Computes( _y = _alpha * _A * _x + _beta * _y ), 
        platform::Parallelisation::NOT_PARALLEL> gemv_n_novector;
	// Compuing _y = _alpha * _A * _x + _beta * _y
  using type = double;type alpha = type(0.7410198214662846);
  type beta = type(0.76681179077718);
  int n = 2;
  int m = 3;
  


  Tensor< type > A = Tensor< type >::FromString(R"(
  	0.7722447692966574 0.1987156815341724 0.005522117123602399 0.8154614284548342;
 0.7068573438476171 0.7290071680409873 0.7712703466859457 0.07404465173409036;
 0.3584657285442726 0.11586905952512971 0.8631034258755935 0.6232981268275579
  	)");

  Tensor< type > x = Tensor< type >::FromString(R"(
    0.3308980248526492; 0.06355835028602363; 0.3109823217156622; 0.32518332202674705; 0.7296061783380641; 0.6375574713552131; 0.8872127425763265; 0.4722149251619493
    )");

  Tensor< type > y = Tensor< type >::FromString(R"(
    0.1195942459383017; 0.713244787222995; 0.7607850486168974; 0.5612771975694962; 0.770967179954561; 0.49379559636439074; 0.5227328293819941; 0.42754101835854963; 0.02541912674409519
    )");

  gemv_n_novector(alpha, A, x, n, beta, y, m);

  Tensor< type > refy = Tensor< type >::FromString(R"(
  0.8659594111816049; 0.713244787222995; 0.7607850486168974; 1.2373816088745355; 0.770967179954561; 0.49379559636439074; 1.3918572009906616; 0.42754101835854963; 0.02541912674409519
  )");



  ASSERT_TRUE( refy.AllClose(y) );
 
}

TEST(blas_gemv, blas_gemv_n_novector6) {

	Blas< double, 
        Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), 
        Computes( _y = _alpha * _A * _x + _beta * _y ), 
        platform::Parallelisation::NOT_PARALLEL> gemv_n_novector;
	// Compuing _y = _alpha * _A * _x + _beta * _y
  using type = double;type alpha = type(0.4715375027365919);
  type beta = type(0.3035549250966767);
  int n = -2;
  int m = -3;
  


  Tensor< type > A = Tensor< type >::FromString(R"(
  	0.10789142699330445 0.03142918568673425 0.6364104112637804 0.3143559810763267;
 0.5085706911647028 0.907566473926093 0.24929222914887494 0.41038292303562973;
 0.7555511385430487 0.22879816549162246 0.07697990982879299 0.289751452913768
  	)");

  Tensor< type > x = Tensor< type >::FromString(R"(
    0.16122128725400442; 0.9296976523425731; 0.808120379564417; 0.6334037565104235; 0.8714605901877177; 0.8036720768991145; 0.18657005888603584; 0.8925589984899778
    )");

  Tensor< type > y = Tensor< type >::FromString(R"(
    0.5393422419156507; 0.8074401551640625; 0.8960912999234932; 0.3180034749718639; 0.11005192452767676; 0.22793516254194168; 0.4271077886262563; 0.8180147659224931; 0.8607305832563434
    )");

  gemv_n_novector(alpha, A, x, n, beta, y, m);

  Tensor< type > refy = Tensor< type >::FromString(R"(
  0.37556997752724497; 0.8074401551640625; 0.8960912999234932; 0.6404090409793313; 0.11005192452767676; 0.22793516254194168; 0.4184653682540592; 0.8180147659224931; 0.8607305832563434
  )");



  ASSERT_TRUE( refy.AllClose(y) );
 
}
