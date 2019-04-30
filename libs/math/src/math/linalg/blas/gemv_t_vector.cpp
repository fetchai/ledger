#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/gemv_t.hpp"
namespace fetch
{
namespace math
{
namespace linalg 
{

template< typename S, uint64_t V >
void Blas< S, Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), Computes( _y = _alpha * T(_A) * _x + _beta * _y ),V >::operator()(type const &alpha, Tensor< type > const &a, Tensor< type > const &x, int const &incx, type const &beta, Tensor< type > &y, int const &incy ) const
{
  int j;
  int kx;
  int lenx;
  int ky;
  int leny;
  type temp;
  int i;
  int jy;
  if( (int(a.height()) == 0) || ((int(a.width()) == 0) || ((alpha == 0.0) && (beta == 1.0))) ) 
  {
    return;
  } 
  
  lenx = int(a.height());
  leny = int(a.width());
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
        
        VectorRegisterType vec_zero(0.0);
        
         
        auto ret_slice = y.data().slice(  0, y.padded_size() );
        memory::TrivialRange range( std::size_t(0), std::size_t(leny) );
        ret_slice.in_parallel().Apply(range, [vec_zero](VectorRegisterType &vw_v_y ){
          
          vw_v_y = vec_zero;  
        });
      }
      else 
      {
        
        VectorRegisterType vec_beta(beta);
        
         
        auto ret_slice = y.data().slice(  0, y.padded_size() );
        auto slice_v_y = y.data().slice( 0, y.padded_size() );
        memory::TrivialRange range( std::size_t(0), std::size_t(leny) );
        ret_slice.in_parallel().Apply(range, [vec_beta](VectorRegisterType const &vr_v_y, VectorRegisterType &vw_v_y ){
          
          vw_v_y = vec_beta * vr_v_y;  
        }, slice_v_y);
      }
    }
    else 
    {
      int iy;
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
  
  jy = -1 + ky;
  if( incx == 1 ) 
  {
    for(j = 0 ; j <  int(a.width()); ++j)
    {
      temp = 0.0;
      
      auto slice_a_j = a.data().slice( a.padded_height() * std::size_t(j), a.padded_height() );
      auto slice_v_x = x.data().slice( 0, x.padded_size() );
      memory::TrivialRange range( std::size_t(0), std::size_t(int(a.height())) );
      temp = slice_a_j.in_parallel().SumReduce(range, [](VectorRegisterType const &vr_a_j, VectorRegisterType const &vr_v_x) {
        return vr_a_j * vr_v_x;  
      }, slice_v_x );
      y[jy] = y[jy] + alpha * temp;
      jy = jy + incy;
    }
  }
  else 
  {
    for(j = 0 ; j <  int(a.width()); ++j)
    {
      int ix;
      temp = 0.0;
      ix = -1 + kx;
      for(i = 0 ; i <  int(a.height()); ++i)
      {
        temp = temp + a(i, j) * x[ix];
        ix = ix + incx;
      }
      
      y[jy] = y[jy] + alpha * temp;
      jy = jy + incy;
    }
  } 
  
  return;
  
};


template class
Blas< double , Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), Computes( _y = _alpha * T(_A) * _x + _beta * _y ), platform::Parallelisation::VECTORISE >;
template class
Blas< float , Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), Computes( _y = _alpha * T(_A) * _x + _beta * _y ), platform::Parallelisation::VECTORISE >;
template class
Blas< double , Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), Computes( _y = _alpha * T(_A) * _x + _beta * _y ), platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING >;
template class
Blas< float , Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ), Computes( _y = _alpha * T(_A) * _x + _beta * _y ), platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING >;

} // namespace linalg
} // namespace math
} // namepsace fetch