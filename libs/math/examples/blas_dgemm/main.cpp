#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/gemm_nn_vector_threaded.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"

using namespace fetch;
using namespace fetch::math::linalg;

int main(int argc, char **argv)
{

  Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C = _alpha * _A * _B + _beta * _C),
       platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>
      gemm_nn_vector_threaded;
  // Compuing _C = _alpha * _A * _B + _beta * _C
  using type = double;

  type alpha = type(1), beta = type(0);

  Matrix<type> A = Matrix<type>(R"(
  0.3745401188473625 0.9507143064099162;
 0.7319939418114051 0.5986584841970366;
 0.15601864044243652 0.15599452033620265
  )");

  Matrix<type> B = Matrix<type>(R"(
  0.05808361216819946 0.8661761457749352 0.6011150117432088;
 0.7080725777960455 0.020584494295802447 0.9699098521619943
  )");

  Matrix<type> C = Matrix<type>(R"(
  0.8324426408004217 0.21233911067827616 0.18182496720710062;
 0.18340450985343382 0.3042422429595377 0.5247564316322378;
 0.43194501864211576 0.2912291401980419 0.6118528947223795
  )");

  Matrix<type> R = Matrix<type>(R"(
  0.6949293726918103 0.3439876897985273 1.14724886031757;
 0.46641050835051406 0.6463587734018926 1.0206573088309918;
 0.11951756833898089 0.1383506929615121 0.24508576903908225
  )");

  gemm_nn_vector_threaded(alpha, A, B, beta, C);

  return 0;
}
