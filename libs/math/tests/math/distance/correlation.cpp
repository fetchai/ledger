#include <iomanip>
#include <iostream>

#include "core/random/lcg.hpp"
#include "math/distance/pearson.hpp"
#include "math/linalg/matrix.hpp"
#include "testing/unittest.hpp"

using namespace fetch::math::distance;
using namespace fetch::math::linalg;

template <typename D>
using _S = fetch::memory::SharedArray<D>;

template <typename D>
using _M = Matrix<D, _S<D>>;

int main()
{
  SCENARIO("Pearson Correlation")
  {
    _M<double> A = _M<double>(R"(1 0 0)");
    EXPECT(Pearson(A, A) == 0);

    /*
        B = _M<double>(R"(0 2 0)");
        EXPECT( Correlation(A,B) == 3 );

        B = _M<double>(R"(1 1 0)");
        EXPECT( Correlation(A,B) == 1 );
        */
  };

  return 0;
}
