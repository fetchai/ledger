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
class Blas< double, Computes( _C <= _alpha * _A * _B + _beta * _C ) > 
{
public:
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

    nrowa = b.height();
    ncola = c.width();
    nrowb = c.width();
    if( (b.height() == 0) || ((a.width() == 0) || (((alpha == zero) || (c.width() == 0)) && (beta == one))) ) 
    {
      return;
    } // endif
    
    if( alpha == zero ) 
    {
      if( beta == zero ) 
      {
        for(j = 0 ; j < a.width(); ++j )
        {
          for(i = 0 ; i < b.height(); ++i )
          {
            c(i, j) = zero;
          }
        }
      }
      else 
      {
        for(j = 0 ; j < a.width(); ++j )
        {
          for(i = 0 ; i < b.height(); ++i )
          {
            c(i, j) = beta * c(i, j);
          }
        }
      } // endif
      
      return;
    } // endif
    
    /* TODO: parallelise over threads */
    for(j = 0 ; j < a.width(); ++j )
    {
      if( beta == zero ) 
      {
        for(i = 0 ; i < b.height(); ++i )
        {
          c(i, j) = zero;
        }
      }
      else if ( beta != one )
      {
        for(i = 0 ; i < b.height(); ++i )
        {
          c(i, j) = beta * c(i, j);
        }
      } // endif
      
      for(l = 0 ; l < c.width(); ++l )
      {
        temp = alpha * b(l, j);
        for(i = 0 ; i < b.height(); ++i )
        {
          c(i, j) = c(i, j) + temp * a(i, l);
        }
      }
    }
    return;
    
  }
};


} // namespace linalg
} // namespace math
} // namepsace fetch
