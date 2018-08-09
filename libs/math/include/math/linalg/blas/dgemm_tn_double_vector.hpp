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
class Blas< double, Computes( _C <= _C = _alpha * T(_A) * _B + _beta * _C ),platform::Parallelisation::VECTORISE>
{
public:
  using vector_register_type = typename Matrix< double >::vector_register_type;

  void operator()(double const &alpha, Matrix< double > const &a, Matrix< double > const &b, double const &beta, Matrix< double > &c ) 
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
          
          vector_register_type vec_zero(0.0);
          
          auto ret_slice = c.data().slice( c.padded_height() * j, c.height());
          ret_slice.in_parallel().Apply([vec_zero](vector_register_type &vw_c ){
            
            vw_c = vec_zero;  
          });
        }
      }
      else 
      {
        for(j = 0 ; j < c.width(); ++j )
        {
          
          vector_register_type vec_beta(beta);
          
          auto ret_slice = c.data().slice( c.padded_height() * j, c.height());
          auto slice_c = c.data().slice( c.padded_height() * j, c.height());
          ret_slice.in_parallel().Apply([vec_beta](vector_register_type const &vr_c, vector_register_type &vw_c ){
            
            vw_c = vec_beta * vr_c;  
          }, slice_c);
        }
      } 
      
      return;
    } 
    
    
    for(j = 0 ; j < c.width(); ++j )
    {  for(i = 0 ; i < c.height(); ++i )
      {
        double temp;
        temp = 0.0;
        
        auto slice_a = a.data().slice( a.padded_height() * i, a.height());
        auto slice_b = b.data().slice( b.padded_height() * j, a.height());
        temp = slice_a.in_parallel().SumReduce([](vector_register_type const &vr_a , vector_register_type const &vr_b ) {
          return vr_a * vr_b;  
        }, slice_b );
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