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
class Blas< double, Computes( _C <= _alpha * T(_A) * _B + _beta * _C ),platform::Parallelisation::NOT_PARALLEL>
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

    nrowa = a.height();
    ncola = c.height();
    nrowb = a.height();
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
      for(i = 0 ; i < c.height(); ++i )
      {
        temp = zero;
        /* TODO: Vectorise loop */
        for(l = 0 ; l < a.height(); ++l )
        {
          temp = temp + a(l, i) * b(l, j);
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