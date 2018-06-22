#include <iostream>
#include <iomanip>

#include <math/linalg/matrix.hpp>
#include <math/distance/hamming.hpp>
#include "core/random/lcg.hpp"
#include "core/unittest.hpp"

using namespace fetch::math::distance;
using namespace fetch::math::linalg;

template <typename D>
using _S = fetch::memory::SharedArray<D>;

template <typename D>
using _M = Matrix<D, _S<D> >;

int main() {
  SCENARIO("Basic info") {
    _M<double> A = _M<double>(R"(1 2; 3 4)");
    EXPECT( Hamming(A,A) == 4 );

    _M<double> B = _M<double>(R"(1 2; 3 2)");
    EXPECT( Hamming(A,B) == 3 );

    A = _M<double>(R"(1 2 3)");
    B = _M<double>(R"(1 2 9)");
    EXPECT( Hamming(A,B) == 2 );    
  };

  return 0;
}


