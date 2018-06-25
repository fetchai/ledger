#ifndef LIBFETCHCORE_MATH_CORRELATION_PEARSON_HPP
#define LIBFETCHCORE_MATH_CORRELATION_PEARSON_HPP

#include"math/correlation/pearson.hpp"
#include"math/linalg/matrix.hpp"
#include"python/fetch_pybind.hpp"

namespace fetch
{
namespace math
{
namespace correlation
{

template< typename A >
inline typename A::type WrapperPearson(A const &a, A const &b) 
{
  if(a.size() != b.size()) {
    throw std::range_error("A and B must have same size");
  }
  
  return Pearson(a,b);  
}


inline void BuildPearsonCorrelation(std::string const &custom_name, pybind11::module &module) {
  using namespace fetch::math::linalg;
  using namespace fetch::memory;  
  
  namespace py = pybind11;
  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperPearson< Matrix< double > >)
    .def(custom_name.c_str(), &WrapperPearson< Matrix< float > >)
    .def(custom_name.c_str(), &WrapperPearson< RectangularArray< double > >)
    .def(custom_name.c_str(), &WrapperPearson<  RectangularArray< float > >)    
    .def(custom_name.c_str(), &WrapperPearson< ShapeLessArray< double > >)    
    .def(custom_name.c_str(), &WrapperPearson< ShapeLessArray< float > >);  
    
};

}
}
}

#endif
