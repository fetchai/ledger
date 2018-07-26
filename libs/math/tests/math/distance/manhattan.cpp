#include <iomanip>
#include <iostream>

#include "core/random/lcg.hpp"
#include "testing/unittest.hpp"
#include <math/distance/manhattan.hpp>
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
    _M<double> A = _M<double>(R"(1 0 0)");
    EXPECT(Manhattan(A, A) == 0);

    _M<double> B = _M<double>(R"(0 1 0)");
    EXPECT(Manhattan(A, B) == 2);

    B = _M<double>(R"(0 2 0)");
    EXPECT(Manhattan(A, B) == 3);

    B = _M<double>(R"(1 1 0)");
    EXPECT(Manhattan(A, B) == 1);
  };

  return 0;
}
