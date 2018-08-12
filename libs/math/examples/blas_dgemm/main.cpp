#include "math/linalg/blas/base.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"
//#include"math/linalg/blas/dgemm.hpp"

using namespace fetch;
using namespace fetch::math::linalg;

int main(int argc, char **argv)
{
  /*
  Blas< double, Computes( _C <= _C = _alpha * _A * _B + _beta * _C
),platform::Parallelisation::VECTORISE |  platform::Parallelisation::THREADING  >
dgemm_nn_double_vector;
// Blas< double, Computes( _C <= _alpha *  _A * T(_B) + _beta * _C
),platform::Parallelisation::NOT_PARALLEL> dgemm_nn_double_vector;
  // Compuing _C <= _alpha * _A * _B + _beta * _C

  double alpha = 1., beta = 2.;

  Matrix< double > A(1000, 1000);
  Matrix< double > B(1000, 1000);
  Matrix< double > C(1000, 1000);

  for(std::size_t i=0; i < 1000; ++i) {
  for(std::size_t j=0; j < 1000; ++j) {
    A(i,j) = B(i,j) = C(i,j) = double(2 * i + j) * 3.2124;
  }
  }

//  std::cout << "Was here? " << std::endl;

  dgemm_nn_double_vector(alpha, A, B, beta, C);
  */
  return 0;
}
