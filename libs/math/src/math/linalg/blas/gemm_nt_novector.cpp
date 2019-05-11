#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/gemm_nt_novector.hpp"
namespace fetch
{
namespace math
{
namespace linalg 
{

template< typename S >
void Blas< S, Signature( _C <= _alpha, _A, _B, _beta, _C ), Computes( _C <= _alpha * _A * T(_B) + _beta * _C ),platform::Parallelisation::NOT_PARALLEL >::operator()(type const &alpha, Tensor< type > const &a, Tensor< type > const &b, type const &beta, Tensor< type > &c ) const
{
  std::size_t i;
  std::size_t j;
  if( (c.height() == 0) || ((c.width() == 0) || (((alpha == 0.0) || (a.width() == 0)) && (beta == 1.0))) ) 
  {
    return;
  } 
  
  if( alpha == 0.0 ) 
  {
    if( beta == 0.0 ) 
    {
      for(j = 0 ; j <  c.width(); ++j)
      {
        for(i = 0 ; i <  c.height(); ++i)
        {
          c(i, j) = 0.0;
        }
      }
    }
    else 
    {
      for(j = 0 ; j <  c.width(); ++j)
      {
        for(i = 0 ; i <  c.height(); ++i)
        {
          c(i, j) = beta * c(i, j);
        }
      }
    } 
    
    return;
  } 
  
  
  for(j = 0 ; j <  c.width(); ++j)
  {  std::size_t l;
    if( beta == 0.0 ) 
    {
      for(i = 0 ; i <  c.height(); ++i)
      {
        c(i, j) = 0.0;
      }
    }
    else if ( beta != 1.0 )
    {
      for(i = 0 ; i <  c.height(); ++i)
      {
        c(i, j) = beta * c(i, j);
      }
    } 
    
    for(l = 0 ; l <  a.width(); ++l)
    {
      type temp;
      temp = alpha * b(j, l);
      for(i = 0 ; i <  c.height(); ++i)
      {
        c(i, j) = c(i, j) + temp * a(i, l);
      }
    }  
    }
  return;
  
};


template class
Blas< double , Signature( _C <= _alpha, _A, _B, _beta, _C ), Computes( _C <= _alpha * _A * T(_B) + _beta * _C ), platform::Parallelisation::NOT_PARALLEL>;

template class
Blas< float , Signature( _C <= _alpha, _A, _B, _beta, _C ), Computes( _C <= _alpha * _A * T(_B) + _beta * _C ), platform::Parallelisation::NOT_PARALLEL>;


} // namespace linalg
} // namespace math
} // namepsace fetch