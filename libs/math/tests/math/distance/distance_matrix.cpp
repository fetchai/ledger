#include <iomanip>
#include <iostream>

#include "core/random/lcg.hpp"
#include "math/distance/distance_matrix.hpp"
#include "testing/unittest.hpp"
#include <math/distance/hamming.hpp>
#include <math/linalg/matrix.hpp>

using namespace fetch::math::distance;
using namespace fetch::math::linalg;

template <typename D>
using _S = fetch::memory::SharedArray<D>;

template <typename D>
using _M = Matrix<D, _S<D>>;

int main()
{
  SCENARIO("Basic info")
  {
    _M<double> A, B, R;

    R.Resize(3, 3);
    A = _M<double>(R"(1 2 3 ; 1 1 1 ; 2 1 2)");
    B = _M<double>(R"(1 2 9 ; 1 0 0 ; 1 2 3)");

    EXPECT(bool(DistanceMatrix(R, A, B, Hamming<double>) ==
                _M<double>(R"( 2 1 3 ; 1 1 1  ; 0 0 0 )")));
  };

  return 0;
}
