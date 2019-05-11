#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/swap_all.hpp"
namespace fetch
{
namespace math
{
namespace linalg 
{

template< typename S, uint64_t V >
void Blas< S, Signature( _x, _y <= _n, _x, _m, _y, _p ), Computes( _x, _y <= _y, _x ),V >::operator()(int const &n, Tensor< type > &dx, int const &incx, Tensor< type > &dy, int const &incy ) const
{
  type dtemp;
  int i;
  if( (incx == 1) && (incy == 1) ) 
  {
    int m;
    int mp1;
    m = n % 3;
    if( m != 0 ) 
    {
      for(i = 0 ; i <  m; ++i)
      {
        dtemp = dx[i];
        dx[i] = dy[i];
        dy[i] = dtemp;
      }
      
      if( n < 3 ) 
      {
        return;
      }
    } 
    
    mp1 = 1 + m;
    for(i = mp1 - 1 ; i <  n; i+=3)
    {
      dtemp = dx[i];
      dx[i] = dy[i];
      dy[i] = dtemp;
      dtemp = dx[1 + i];
      dx[1 + i] = dy[1 + i];
      dy[1 + i] = dtemp;
      dtemp = dx[2 + i];
      dx[2 + i] = dy[2 + i];
      dy[2 + i] = dtemp;
    }
  }
  else 
  {
    int ix;
    int iy;
    ix = 0;
    iy = 0;
    if( incx < 0 ) 
    {
      ix = (1 + (-n)) * incx;
    } 
    
    if( incy < 0 ) 
    {
      iy = (1 + (-n)) * incy;
    } 
    
    for(i = 0 ; i <  n; ++i)
    {
      dtemp = dx[ix];
      dx[ix] = dy[iy];
      dy[iy] = dtemp;
      ix = ix + incx;
      iy = iy + incy;
    }
  } 
  
  return;
  
};


template class
Blas< double , Signature( _x, _y <= _n, _x, _m, _y, _p ), Computes( _x, _y <= _y, _x ), platform::Parallelisation::NOT_PARALLEL >;
template class
Blas< float , Signature( _x, _y <= _n, _x, _m, _y, _p ), Computes( _x, _y <= _y, _x ), platform::Parallelisation::NOT_PARALLEL >;
template class
Blas< double , Signature( _x, _y <= _n, _x, _m, _y, _p ), Computes( _x, _y <= _y, _x ), platform::Parallelisation::THREADING >;
template class
Blas< float , Signature( _x, _y <= _n, _x, _m, _y, _p ), Computes( _x, _y <= _y, _x ), platform::Parallelisation::THREADING >;
template class
Blas< double , Signature( _x, _y <= _n, _x, _m, _y, _p ), Computes( _x, _y <= _y, _x ), platform::Parallelisation::VECTORISE >;
template class
Blas< float , Signature( _x, _y <= _n, _x, _m, _y, _p ), Computes( _x, _y <= _y, _x ), platform::Parallelisation::VECTORISE >;
template class
Blas< double , Signature( _x, _y <= _n, _x, _m, _y, _p ), Computes( _x, _y <= _y, _x ), platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING >;
template class
Blas< float , Signature( _x, _y <= _n, _x, _m, _y, _p ), Computes( _x, _y <= _y, _x ), platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING >;

} // namespace linalg
} // namespace math
} // namepsace fetch