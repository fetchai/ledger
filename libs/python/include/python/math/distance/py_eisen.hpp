#pragma once

#include"math/distance/eisen.hpp"
#include"math/linalg/matrix.hpp"
#include"python/fetch_pybind.hpp"

namespace fetch
{
namespace math
{
namespace distance
{


template< typename A >
inline typename A::type WrapperEisen(A const &a, A const &b) 
{
  if(a.size() != b.size()) {
    throw std::range_error("A and B must have same size");
  }
  
  return Eisen(a,b);  
}


inline void BuildEisenDistance(std::string const &custom_name, pybind11::module &module) {
  using namespace fetch::math::linalg;
  using namespace fetch::memory;  
  
  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperEisen< Matrix< double > >)
    .def(custom_name.c_str(), &WrapperEisen< Matrix< float > >)
    .def(custom_name.c_str(), &WrapperEisen< RectangularArray< double > >)
    .def(custom_name.c_str(), &WrapperEisen<  RectangularArray< float > >)    
    .def(custom_name.c_str(), &WrapperEisen< ShapeLessArray< double > >)    
    .def(custom_name.c_str(), &WrapperEisen< ShapeLessArray< float > >);  
  
};

}
}
}

