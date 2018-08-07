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
class Blas< double, Computes( _C <= _alpha * T(_A) * T(_B) + _beta * _C ),platform::Parallelisation::VECTORISE>
{
public:
  using vector_register_type = typename Matrix< double >::vector_register_type;

  void operator()(double const &alpha, Matrix< double > const &a, Matrix< double > const &b, double const &beta, Matrix< double > &c ) 
  {
    constexpr double one = 1.0;
    constexpr double zero = 0.0;
    double temp;
    std::size_t i;
    std::size_t j;
    std::size_t l;
    std::size_t ncola;
    std::size_t nrowa;
    std::size_t nrowb;

    nrowa = a.height();
    ncola = c.height();
    nrowb = c.width();
    if( (c.height() == 0) || ((c.width() == 0) || (((alpha == zero) || (a.height() == 0)) && (beta == one))) ) 
    {
      return;
    } // endif
    
    if( alpha == zero ) 
    {
      if( beta == zero ) 
      {
        
        for(j = 0 ; j < c.width(); ++j )
        {
          
          
          vector_register_type vec_zero(zero);
          
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
      } // endif
      
      return;
    } // endif
    
    
    for(j = 0 ; j < c.width(); ++j )
    {  
      for(i = 0 ; i < c.height(); ++i )
      {
        temp = zero;
        
        for(l = 0 ; l < a.height(); ++l )
        {
          temp = temp + a(l, i) * b(j, l);
        }
        
        if( beta == zero ) 
        {
          c(i, j) = alpha * temp;
        }
        else 
        {
          c(i, j) = alpha * temp + beta * c(i, j);
        } // endif
      }  
      }
    
    return;
    
  }
};


} // namespace linalg
} // namespace math
} // namepsace fetch