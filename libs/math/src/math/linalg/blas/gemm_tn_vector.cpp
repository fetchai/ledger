//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/gemm_tn_vector.hpp"
namespace fetch
{
namespace math
{
namespace linalg 
{

template< typename S >
void Blas< S, Signature( _C <= _alpha, _A, _B, _beta, _C ), Computes( _C <=  _alpha * T(_A) * _B + _beta * _C ),platform::Parallelisation::VECTORISE >::operator()(Type const &alpha, Tensor< Type > const &a, Tensor< Type > const &b, Type const &beta, Tensor< Type > &c ) const
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
      for(j = 0 ; j <  c.width(); ++j)
      {
        
        VectorRegisterType vec_zero(0.0);
        
         
        auto ret_slice = c.data().slice( c.padded_height() * j, c.height());
        memory::TrivialRange range(std::size_t(0), std::size_t(c.height()));
        ret_slice.in_parallel().Apply(range, [vec_zero](VectorRegisterType &vw_c_j ){
          
          vw_c_j = vec_zero;  
        });
      }
    }
    else 
    {
      for(j = 0 ; j <  c.width(); ++j)
      {
        
        VectorRegisterType vec_beta(beta);
        
         
        auto ret_slice = c.data().slice( c.padded_height() * j, c.height());
        auto slice_c_j = c.data().slice( c.padded_height() * std::size_t(j), c.padded_height() );
        memory::TrivialRange range(std::size_t(0), std::size_t(c.height()));
        ret_slice.in_parallel().Apply(range, [vec_beta](VectorRegisterType const &vr_c_j, VectorRegisterType &vw_c_j ){
          
          vw_c_j = vec_beta * vr_c_j;  
        }, slice_c_j);
      }
    } 
    
    return;
  } 
  
  
  for(j = 0 ; j <  c.width(); ++j)
  {  for(i = 0 ; i <  c.height(); ++i)
    {
      Type temp;
      temp = 0.0;
      
      auto slice_a_i = a.data().slice( a.padded_height() * std::size_t(i), a.padded_height() );
      auto slice_b_j = b.data().slice( b.padded_height() * std::size_t(j), b.padded_height() );
      memory::TrivialRange range( std::size_t(0), std::size_t(a.height()) );
      temp = slice_a_i.in_parallel().SumReduce(range, [](VectorRegisterType const &vr_a_i, VectorRegisterType const &vr_b_j) {
        return vr_a_i * vr_b_j;  
      }, slice_b_j );
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


template class
Blas< double , Signature( _C <= _alpha, _A, _B, _beta, _C ), Computes( _C <=  _alpha * T(_A) * _B + _beta * _C ), platform::Parallelisation::VECTORISE>;

template class
Blas< float , Signature( _C <= _alpha, _A, _B, _beta, _C ), Computes( _C <=  _alpha * T(_A) * _B + _beta * _C ), platform::Parallelisation::VECTORISE>;


} // namespace linalg
} // namespace math
} // namepsace fetch