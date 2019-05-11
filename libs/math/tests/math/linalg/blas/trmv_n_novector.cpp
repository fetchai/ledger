#include <gtest/gtest.h>

#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/trmv_n.hpp"


using namespace fetch;
using namespace fetch::math;
using namespace fetch::math::linalg;

TEST(blas_trmv, blas_trmv_n_novector1) {

	Blas< double, 
        Signature( _x <= _A, _x, _n ), 
        Computes( _x <= _A * _x ), 
        platform::Parallelisation::NOT_PARALLEL> trmv_n_novector;
	// Compuing _x <= _A * _x
  using type = double;
  
  int n = 1;
  
  


  Tensor< type > A = Tensor< type >::FromString(R"(
  	0.3745401188473625 0.9507143064099162;
 0.7319939418114051 0.5986584841970366;
 0.15601864044243652 0.15599452033620265
  	)");

  Tensor< type > x = Tensor< type >::FromString(R"(
    0.05808361216819946; 0.8661761457749352
    )");

  trmv_n_novector(A, x, n);

  Tensor< type > refx = Tensor< type >::FromString(R"(
  0.05808361216819946; 0.8661761457749352
  )");



  ASSERT_TRUE( refx.AllClose(x) );

 
}
