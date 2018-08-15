#pragma once

#include"math/linalg/prototype.hpp"
#include"math/linalg/matrix.hpp"
#include"math/linalg/blas/base.hpp"

namespace fetch
{
namespace math
{
namespace linalg 
{


template<typename S>
class Blas< S, 
            Signature( L(_C) <= _alpha, L(_A), _beta, L(_C) ),
            Computes( _C = _alpha * T(_A) * _A + _beta * _C ), 
            platform::Parallelisation::VECTORISE>
{
public:
  using type = S;
  using vector_register_type = typename Matrix< type >::vector_register_type;
  
  void operator()(type const &alpha, Matrix< type > const &a, type const &beta, Matrix< type > &c ) const;
};

} // namespace linalg
} // namespace math
} // namepsace fetch