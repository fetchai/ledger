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


template<>
class Blas< float, Computes( _C <= _C = _alpha * T(_A) * _B + _beta * _C ),platform::Parallelisation::NOT_PARALLEL>
{
public:
  using vector_register_type = typename Matrix< float >::vector_register_type;

  void operator()(float const &alpha, Matrix< float > const &a, Matrix< float > const &b, float const &beta, Matrix< float > &c ) 
  {
    std::size_t i;
    std::size_t j;
    if( (c.height() == 0) || ((c.width() == 0) || (((alpha == 0.0) || (a.height() == 0)) && (beta == 1.0))) ) 
    {
      return;
    } 
    
    if( alpha == 0.0 ) 
    {
      if( beta == 0.0 ) 
      {
        for(j = 0 ; j < c.width(); ++j )
        {
          for(i = 0 ; i < c.height(); ++i )
          {
            c(i, j) = 0.0;
          }
        }
      }
      else 
      {
        for(j = 0 ; j < c.width(); ++j )
        {
          for(i = 0 ; i < c.height(); ++i )
          {
            c(i, j) = beta * c(i, j);
          }
        }
      } 
      
      return;
    } 
    
    
    for(j = 0 ; j < c.width(); ++j )
    {  for(i = 0 ; i < c.height(); ++i )
      {
        float temp;
        std::size_t l;
        temp = 0.0;
        for(l = 0 ; l < a.height(); ++l )
        {
          temp = temp + a(l, i) * b(l, j);
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
    
  }
};


} // namespace linalg
} // namespace math
} // namepsace fetch