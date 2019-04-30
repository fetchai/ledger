#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/gemm_tt_novector.hpp"
namespace fetch
{
namespace math
{
namespace linalg 
{

template< typename S >
void Blas< S, Signature( _C <= _alpha, _A, _B, _beta, _C ), Computes( _C = _alpha * T(_A) * T(_B) + _beta * _C ),platform::Parallelisation::NOT_PARALLEL >::operator()(type const &alpha, Tensor< type > const &a, Tensor< type > const &b, type const &beta, Tensor< type > &c ) const
{
  std::size_t j;
  std::size_t i;
  if( (c.height() == 0) || ((c.width() == 0) || (((alpha == 0.0) || (a.height() == 0)) && (beta == 1.0))) ) 
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
  {  for(i = 0 ; i <  c.height(); ++i)
    {
      type temp;
      std::size_t l;
      temp = 0.0;
      for(l = 0 ; l <  a.height(); ++l)
      {
        temp = temp + a(l, i) * b(j, l);
      }
      
      if( beta == 0.0 ) 
      {
        c(i, j) = alpha * temp;
      }
      else 
      {
        c(i, j) = alpha * temp + beta * c(i, j);
      }
    }  
    }
  return;
  
};


template class
Blas< double , Signature( _C <= _alpha, _A, _B, _beta, _C ), Computes( _C = _alpha * T(_A) * T(_B) + _beta * _C ), platform::Parallelisation::NOT_PARALLEL>;

template class
Blas< float , Signature( _C <= _alpha, _A, _B, _beta, _C ), Computes( _C = _alpha * T(_A) * T(_B) + _beta * _C ), platform::Parallelisation::NOT_PARALLEL>;


} // namespace linalg
} // namespace math
} // namepsace fetch