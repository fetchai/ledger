#pragma once

#include"math/statistics/standard_deviation.hpp"
#include"math/linalg/matrix.hpp"
#include"python/fetch_pybind.hpp"

namespace fetch
{
namespace math
{
namespace statistics
{


template< typename A >
inline typename A::type WrapperStandardDeviation(A const &a) 
{
  return StandardDeviation(a);
}


inline void BuildStandardDeviationStatistics(std::string const &custom_name, pybind11::module &module) {
  using namespace fetch::math::linalg;
  using namespace fetch::memory;  
  
  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperStandardDeviation< Matrix< double > >)
    .def(custom_name.c_str(), &WrapperStandardDeviation< Matrix< float > >)
    .def(custom_name.c_str(), &WrapperStandardDeviation< RectangularArray< double > >)
    .def(custom_name.c_str(), &WrapperStandardDeviation<  RectangularArray< float > >)    
    .def(custom_name.c_str(), &WrapperStandardDeviation< ShapeLessArray< double > >)    
    .def(custom_name.c_str(), &WrapperStandardDeviation< ShapeLessArray< float > >);  
  
};

}
}
}

