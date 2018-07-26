#ifndef LIBFETCHCORE_MATH_DISTANCE_DISTANCE_MATRIX_HPP
#define LIBFETCHCORE_MATH_DISTANCE_DISTANCE_MATRIX_HPP

#include "core/byte_array/byte_array.hpp"
#include "math/distance/distance_matrix.hpp"
#include "math/distance/eisen.hpp"
#include "math/distance/euclidean.hpp"
#include "math/distance/hamming.hpp"
#include "math/distance/jaccard.hpp"
#include "math/distance/manhattan.hpp"
#include "math/distance/pearson.hpp"
#include "math/linalg/matrix.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace distance {

template <typename A>
inline A WrapperDistanceMatrix(A const &a, A const &b,
                               std::string const &method = "eucludian")
{
  if (a.width() != b.width())
  {
    throw std::range_error("A and B must have same width");
  }

  A ret;
  ret.Resize(a.height(), b.height());
  if (method == "euclidean")
  {
    DistanceMatrix(ret, a, b, Euclidean<typename A::type>);
  }
  else if (method == "hamming")
  {
    DistanceMatrix(ret, a, b, Hamming<typename A::type>);
  }
  else if (method == "manhattan")
  {
    DistanceMatrix(ret, a, b, Manhattan<typename A::type>);
  }
  else if (method == "pearson")
  {
    DistanceMatrix(ret, a, b, Pearson<typename A::type>);
  }
  else if (method == "eisen")
  {
    DistanceMatrix(ret, a, b, Eisen<typename A::type>);
  }
  else if (method == "cosine")
  {
    DistanceMatrix(ret, a, b, Eisen<typename A::type>);
  }
  else if (method == "jaccard")
  {
    DistanceMatrix(ret, a, b, Jaccard<typename A::type>);
  }
  else if (method == "genelralised jaccard")
  {
    DistanceMatrix(ret, a, b, GeneralisedJaccard<typename A::type>);
  }
  else if (method == "braycurtis")
  {
    DistanceMatrix(ret, a, b, Braycurtis<typename A::type>);
  }
  else
  {
    throw std::runtime_error(
        "Snap... didn't see that one coming, but hey, why don't you open a "
        "ticket for this methods? Available methods are euclidean, hamming, "
        "manhattan, pearson, eisen, cosine, jaccard, genelralised jaccard and "
        "braycurtis");
  }
  return ret;
}

inline void BuildDistanceMatrixDistance(std::string const &custom_name,
                                        pybind11::module & module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperDistanceMatrix<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperDistanceMatrix<Matrix<float>>)
      .def(custom_name.c_str(),
           &WrapperDistanceMatrix<RectangularArray<double>>)
      .def(custom_name.c_str(),
           &WrapperDistanceMatrix<RectangularArray<float>>);
  //    .def(custom_name.c_str(), &WrapperDistanceMatrix< ShapeLessArray< double
  //    > >) .def(custom_name.c_str(), &WrapperDistanceMatrix< ShapeLessArray<
  //    float > >);
};

}  // namespace distance
}  // namespace math
}  // namespace fetch

#endif
