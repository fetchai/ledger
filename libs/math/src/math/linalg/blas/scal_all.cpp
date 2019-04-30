#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/scal_all.hpp"
namespace fetch
{
namespace math
{
namespace linalg 
{

template< typename S, uint64_t V >
void Blas< S, Signature( _x <= _n, _alpha, _x, _m ), Computes( _x = _alpha * _x ),V >::operator()(int const &n, type const &da, Tensor< type > &dx, int const &incx ) const
{
  int i;
  if( (n <= 0) || (incx <= 0) ) 
  {
    return;
  } 
  
  if( incx == 1 ) 
  {
    int m;
    int mp1;
    m = n % 5;
    if( m != 0 ) 
    {
      for(i = 0 ; i <  m; ++i)
      {
        dx[i] = da * dx[i];
      }
      
      if( n < 5 ) 
      {
        return;
      }
    } 
    
    mp1 = 1 + m;
    for(i = mp1 - 1 ; i <  n; i+=5)
    {
      dx[i] = da * dx[i];
      dx[1 + i] = da * dx[1 + i];
      dx[2 + i] = da * dx[2 + i];
      dx[3 + i] = da * dx[3 + i];
      dx[4 + i] = da * dx[4 + i];
    }
  }
  else 
  {
    int nincx;
    nincx = n * incx;
    for(i = 0 ; i <  nincx; i+=incx)
    {
      dx[i] = da * dx[i];
    }
  } 
  
  return;
  
};


template class
Blas< double , Signature( _x <= _n, _alpha, _x, _m ), Computes( _x = _alpha * _x ), platform::Parallelisation::NOT_PARALLEL >;
template class
Blas< float , Signature( _x <= _n, _alpha, _x, _m ), Computes( _x = _alpha * _x ), platform::Parallelisation::NOT_PARALLEL >;
template class
Blas< double , Signature( _x <= _n, _alpha, _x, _m ), Computes( _x = _alpha * _x ), platform::Parallelisation::THREADING >;
template class
Blas< float , Signature( _x <= _n, _alpha, _x, _m ), Computes( _x = _alpha * _x ), platform::Parallelisation::THREADING >;
template class
Blas< double , Signature( _x <= _n, _alpha, _x, _m ), Computes( _x = _alpha * _x ), platform::Parallelisation::VECTORISE >;
template class
Blas< float , Signature( _x <= _n, _alpha, _x, _m ), Computes( _x = _alpha * _x ), platform::Parallelisation::VECTORISE >;
template class
Blas< double , Signature( _x <= _n, _alpha, _x, _m ), Computes( _x = _alpha * _x ), platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING >;
template class
Blas< float , Signature( _x <= _n, _alpha, _x, _m ), Computes( _x = _alpha * _x ), platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING >;

} // namespace linalg
} // namespace math
} // namepsace fetch