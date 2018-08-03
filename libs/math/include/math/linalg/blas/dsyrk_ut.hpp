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
class Blas< double, Computes( _C <= _alpha * T(_A) * _A + _beta * _C ) > 
{
public:
  void operator()(double const &alpha, Matrix< double > const &a, double const &beta, Matrix< double > &c ) 
  {
    constexpr bool upper = true;
    constexpr double one = 1.0;
    constexpr double zero = 0.0;
    constexpr int info = 0;
    double temp;
    std::size_t i;
    std::size_t j;
    std::size_t l;
    std::size_t nrowa;

    nrowa = a.width();
    if( (a.height() == 0) || (((alpha == zero) || (a.width() == 0)) && (beta == one)) ) 
    {
      return;
    } // endif
    
    if( alpha == zero ) 
    {
      if( beta == zero ) 
      {
        for(j = 0 ; j < a.height(); ++j )
        {
          for(i = 0 ; i < j; ++i )
          {
            c(i, j) = zero;
          }
        }
      }
      else 
      {
        for(j = 0 ; j < a.height(); ++j )
        {
          for(i = 0 ; i < j; ++i )
          {
            c(i, j) = beta * c(i, j);
          }
        }
      } // endif
      
      return;
    } // endif
    
    /* TODO: parallelise over threads */
    for(j = 0 ; j < a.height(); ++j )
    {
      for(i = 0 ; i < j; ++i )
      {
        temp = zero;
        for(l = 0 ; l < a.width(); ++l )
        {
          temp = temp + a(l, i) * a(l, j);
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