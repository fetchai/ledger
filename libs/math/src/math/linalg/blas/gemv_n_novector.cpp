#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/gemv_n.hpp"
namespace fetch
{
namespace math
{
namespace linalg 
{

template< typename S, uint64_t V >
void Blas< S, Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), Computes( _y = _alpha * _A * _x + _beta * _y ),V >::operator()(type const &alpha, Tensor< type > const &a, Tensor< type > const &x, int const &incx, type const &beta, Tensor< type > &y, int const &incy ) const
{
  int j;
  int kx;
  int lenx;
  int iy;
  int jx;
  int ky;
  int leny;
  type temp;
  int i;
  if( (int(a.height()) == 0) || ((int(a.width()) == 0) || ((alpha == 0.0) && (beta == 1.0))) ) 
  {
    return;
  } 
  
  lenx = int(a.width());
  leny = int(a.height());
  if( incx > 0 ) 
  {
    kx = 1;
  }
  else 
  {
    kx = 1 + (-(-1 + lenx) * incx);
  } 
  
  if( incy > 0 ) 
  {
    ky = 1;
  }
  else 
  {
    ky = 1 + (-(-1 + leny) * incy);
  } 
  
  if( beta != 1.0 ) 
  {
    if( incy == 1 ) 
    {
      if( beta == 0.0 ) 
      {
        for(i = 0 ; i <  leny; ++i)
        {
          y[i] = 0.0;
        }
      }
      else 
      {
        for(i = 0 ; i <  leny; ++i)
        {
          y[i] = beta * y[i];
        }
      }
    }
    else 
    {
      iy = -1 + ky;
      if( beta == 0.0 ) 
      {
        for(i = 0 ; i <  leny; ++i)
        {
          y[iy] = 0.0;
          iy = iy + incy;
        }
      }
      else 
      {
        for(i = 0 ; i <  leny; ++i)
        {
          y[iy] = beta * y[iy];
          iy = iy + incy;
        }
      }
    }
  } 
  
  if( alpha == 0.0 ) 
  {
    return;
  } 
  
  jx = -1 + kx;
  if( incy == 1 ) 
  {
    for(j = 0 ; j <  int(a.width()); ++j)
    {
      temp = alpha * x[jx];
      for(i = 0 ; i <  int(a.height()); ++i)
      {
        y[i] = y[i] + temp * a(i, j);
      }
      
      jx = jx + incx;
    }
  }
  else 
  {
    for(j = 0 ; j <  int(a.width()); ++j)
    {
      temp = alpha * x[jx];
      iy = -1 + ky;
      for(i = 0 ; i <  int(a.height()); ++i)
      {
        y[iy] = y[iy] + temp * a(i, j);
        iy = iy + incy;
      }
      
      jx = jx + incx;
    }
  } 
  
  return;
  
};


template class
Blas< double , Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), Computes( _y = _alpha * _A * _x + _beta * _y ), platform::Parallelisation::NOT_PARALLEL >;
template class
Blas< float , Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), Computes( _y = _alpha * _A * _x + _beta * _y ), platform::Parallelisation::NOT_PARALLEL >;
template class
Blas< double , Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), Computes( _y = _alpha * _A * _x + _beta * _y ), platform::Parallelisation::THREADING >;
template class
Blas< float , Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), Computes( _y = _alpha * _A * _x + _beta * _y ), platform::Parallelisation::THREADING >;

} // namespace linalg
} // namespace math
} // namepsace fetch