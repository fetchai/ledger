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
class Blas< double, Computes( _C <= _alpha * _A * T(_B) + _beta * _C ),platform::Parallelisation::NOT_PARALLEL>
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

    nrowa = c.height();
    ncola = a.width();
    nrowb = c.width();
    if( (c.height() == 0) || ((c.width() == 0) || (((alpha == zero) || (a.width() == 0)) && (beta == one))) ) 
    {
      return;
    } // endif
    
    if( alpha == zero ) 
    {
      if( beta == zero ) 
      {
        for(j = 0 ; j < c.width(); ++j )
        {
          /* TODO: Vectorise loop */
          for(i = 0 ; i < c.height(); ++i )
          {
            c(i, j) = zero;
          }
        }
      }
      else 
      {
        for(j = 0 ; j < c.width(); ++j )
        {
          /* TODO: Vectorise loop */
          for(i = 0 ; i < c.height(); ++i )
          {
            c(i, j) = beta * c(i, j);
          }
        }
      } // endif
      
      return;
    } // endif
    
    /* TODO: parallelise over threads */
    for(j = 0 ; j < c.width(); ++j )
    {
      if( beta == zero ) 
      {
        /* TODO: Vectorise loop */
        for(i = 0 ; i < c.height(); ++i )
        {
          c(i, j) = zero;
        }
      }
      else if ( beta != one )
      {
        /* TODO: Vectorise loop */
        for(i = 0 ; i < c.height(); ++i )
        {
          c(i, j) = beta * c(i, j);
        }
      } // endif
      
      for(l = 0 ; l < a.width(); ++l )
      {
        temp = alpha * b(j, l);
        /* TODO: Vectorise loop */
        for(i = 0 ; i < c.height(); ++i )
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