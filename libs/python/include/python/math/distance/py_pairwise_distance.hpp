#ifndef LIBFETCHCORE_MATH_DISTANCE_PAIRWISE_DISTANCE_HPP
#define LIBFETCHCORE_MATH_DISTANCE_PAIRWISE_DISTANCE_HPP

#include"math/distance/pairwise_distance.hpp"
#include"math/linalg/matrix.hpp"
#include"python/fetch_pybind.hpp"
#include"core/byte_array/referenced_byte_array.hpp"

namespace fetch
{
namespace math
{
namespace distance
{


template< typename A >
inline A WrapperPairWiseDistance(A const &a, std::string const &method = "eucludian") 
{
  
  A ret;  
  ret.Resize(1, a.height() * (a.height() - 1) / 2 );
  if(method == "euclidean") {
    PairWiseDistance(ret, a, Euclidean< typename A::type > );
  } else if(method == "hamming") {
    PairWiseDistance(ret, a, Hamming< typename A::type > );
  } else if(method == "manhattan") {
    PairWiseDistance(ret, a, Manhattan< typename A::type > );
  } else if(method == "pearson") {
    PairWiseDistance(ret, a, Pearson< typename A::type > ); 
  } else if(method == "eisen") {
    PairWiseDistance(ret, a, Eisen< typename A::type > );
  } else if(method == "cosine") {
    PairWiseDistance(ret, a, Eisen< typename A::type > );
  } else if(method == "jaccard") {
    PairWiseDistance(ret, a, Jaccard< typename A::type > );
  } else if(method == "genelralised jaccard") {
    PairWiseDistance(ret, a, GeneralisedJaccard< typename A::type > );
  } else if(method == "braycurtis") {
    PairWiseDistance(ret, a, Braycurtis< typename A::type > );
  } else {
    throw std::runtime_error("Snap... didn't see that one coming, but hey, why don't you open a ticket for this methods? Available methods are euclidean, hamming, manhattan, pearson, eisen, cosine, jaccard, genelralised jaccard and braycurtis");
  }
  return ret;
}


inline void BuildPairWiseDistanceDistance(std::string const &custom_name, pybind11::module &module) {
  using namespace fetch::math::linalg;
  using namespace fetch::memory;  
  
  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperPairWiseDistance< Matrix< double > >)
    .def(custom_name.c_str(), &WrapperPairWiseDistance< Matrix< float > >)
    .def(custom_name.c_str(), &WrapperPairWiseDistance< RectangularArray< double > >)
    .def(custom_name.c_str(), &WrapperPairWiseDistance<  RectangularArray< float > >);
    //    .def(custom_name.c_str(), &WrapperPairWiseDistance< ShapeLessArray< double > >)    
    //    .def(custom_name.c_str(), &WrapperPairWiseDistance< ShapeLessArray< float > >);  
  
};

}
}
}

#endif
