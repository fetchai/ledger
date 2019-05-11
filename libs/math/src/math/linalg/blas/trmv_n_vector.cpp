#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/trmv_n.hpp"
namespace fetch
{
namespace math
{
namespace linalg 
{

template< typename S, uint64_t V >
void Blas< S, Signature( _x <= _A, _x, _n ), Computes( _x <= _A * _x ),V >::operator()(Tensor< type > const &a, Tensor< type > &x, int const &incx ) const
{
  type temp;
  int i;
  int j;
  int kx;
  if( incx <= 0 ) 
  {
    kx = 1 + (-(-1 + int(a.width())) * incx);
  }
  else if ( incx != 1 )
  {
    kx = 1;
  } 
  
  if( incx == 1 ) 
  {
    for(j = int(a.width()) - 1 ; j <  1; j+=- 1)
    {
      if( x[j] != 0.0 ) 
      {
        temp = x[j];
        
        VectorRegisterType vec_temp(temp);
        
         
        auto ret_slice = x.data().slice(  0, x.padded_size() );
        auto slice_v_x = x.data().slice( 0, x.padded_size() );
        auto slice_a_j = a.data().slice( a.padded_height() * std::size_t(j), a.padded_height() );
        memory::TrivialRange range( std::size_t(int(a.width()) - 1), std::size_t(j + 1 + 1) );
        ret_slice.in_parallel().Apply(range, [vec_temp](VectorRegisterType const &vr_v_x, VectorRegisterType const &vr_a_j, VectorRegisterType &vw_v_x ){
          
          vw_v_x = vr_v_x + vec_temp * vr_a_j;  
        }, slice_v_x, slice_a_j);
        x[j] = x[j] * a(j, j);
      }
    }
  }
  else 
  {
    int jx;
    kx = kx + (-1 + int(a.width())) * incx;
    jx = -1 + kx;
    for(j = int(a.width()) - 1 ; j <  1; j+=- 1)
    {
      if( x[jx] != 0.0 ) 
      {
        int ix;
        temp = x[jx];
        ix = -1 + kx;
        for(i = int(a.width()) - 1 ; i <  j + 1 + 1; i+=- 1)
        {
          x[ix] = x[ix] + temp * a(i, j);
          ix = ix + (-incx);
        }
        
        x[jx] = x[jx] * a(j, j);
      } 
      
      jx = jx + (-incx);
    }
  } 
  
  return;
  
};


template class
Blas< double , Signature( _x <= _A, _x, _n ), Computes( _x <= _A * _x ), platform::Parallelisation::VECTORISE >;
template class
Blas< float , Signature( _x <= _A, _x, _n ), Computes( _x <= _A * _x ), platform::Parallelisation::VECTORISE >;
template class
Blas< double , Signature( _x <= _A, _x, _n ), Computes( _x <= _A * _x ), platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING >;
template class
Blas< float , Signature( _x <= _A, _x, _n ), Computes( _x <= _A * _x ), platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING >;

} // namespace linalg
} // namespace math
} // namepsace fetch