#include "math/meta/math_type_traits.hpp"

namespace fetch
{
namespace math
{

template< typename Type >
IsFetchMathPrimitive< Type, void > Arccos(Type const& x, Type& out)
{
  throw std::runtime_error("Primitive for Arccos not implemented yet.");
}

namespace details
{
  
template< typename ArrayType >
struct ArccosKernel
{
  using VectorRegisterType = typename ArrayType::VectorRegisterType;
  void operator()(VectorRegisterType const &vec_x,VectorRegisterType &vec_out)   
  { 
    throw std::runtime_error("kernel for Arccos not implemented");
  };
};


template< typename ArrayType >
IsFetchArrayType< ArrayType, void > ArccosFallback(ArrayType const& x, ArrayType& out)
{
  auto it1 = x.begin();
  auto it2 = out.begin();

  while (it1.is_valid())
  {
    Arccos(*it1, *it2);
    
    ++it1;
    ++it2; 
  }
}

template< typename ArrayType >
IsFetchArrayType< ArrayType, void > ArccosVectorise(ArrayType const& x, ArrayType& out)
{
  ArccosKernel< ArrayType > kernel;

  auto range = TrivialRange(0, out.size());  
  out.data().in_parallel().Apply(range, kernel, x.data());  

}

template< typename ArrayType >
IsFetchArrayType< ArrayType, void > ArccosColWiseVectorise(ArrayType const& x, ArrayType& out)
{
  ArccosKernel< ArrayType > kernel;  

  auto ph = x.padded_height();
  auto start = ph;
  for(uint64_t j=0; j < remaining_volume; ++j)
  {
    auto range = Range(start, start + x.height());  
    start += ph;
    // TODO: Check this expression
    out.data().in_parallel().Apply(range, kernel, x.data());      
  }

}
} // namespace details

template< typename ArrayType >
IsFetchArrayType< ArrayType, void > Arccos(ArrayType const& x, ArrayType& out, bool vectorise = true)
{
  if((x.shape() != v.shape()))
  {
    throw std::runtime_error("Array shape mismatch in arccos.");
  }

  if(vectorise)
  {
    if(x.height() == x.padded_height())
    {
      details::ArccosVectorise(x, out);
    }
    else
    {
      details::ArccosColWiseVectorise(x, out);      
    }
  }
  else
  {
    details::ArccosFallback(x, out);    
  }
}

}
}