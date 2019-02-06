#pragma once
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

#include "core/byte_array/byte_array.hpp"
#include "math/distance/distance_matrix.hpp"
#include "math/distance/eisen.hpp"
#include "math/distance/euclidean.hpp"
#include "math/distance/hamming.hpp"
#include "math/distance/jaccard.hpp"
#include "math/distance/manhattan.hpp"
#include "math/distance/pearson.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace distance {

template <typename A>
inline A WrapperDistanceMatrix(A const &a, A const &b, std::string const &method = "eucludian")
{
  if (a.width() != b.width())
  {
    throw std::range_error("A and B must have same width");
  }

  A ret;
  ret.Resize(a.height(), b.height());
  if (method == "euclidean")
  {
    DistanceMatrix(ret, a, b, Euclidean<typename A::Type>);
  }
  else if (method == "hamming")
  {
    DistanceMatrix(ret, a, b, Hamming<typename A::Type>);
  }
  else if (method == "manhattan")
  {
    DistanceMatrix(ret, a, b, Manhattan<typename A::Type>);
  }
  else if (method == "pearson")
  {
    DistanceMatrix(ret, a, b, Pearson<typename A::Type>);
  }
  else if (method == "eisen")
  {
    DistanceMatrix(ret, a, b, Eisen<typename A::Type>);
  }
  else if (method == "cosine")
  {
    DistanceMatrix(ret, a, b, Eisen<typename A::Type>);
  }
  else if (method == "jaccard")
  {
    DistanceMatrix(ret, a, b, Jaccard<typename A::Type>);
  }
  else if (method == "genelralised jaccard")
  {
    DistanceMatrix(ret, a, b, GeneralisedJaccard<typename A::Type>);
  }
  else if (method == "braycurtis")
  {
    DistanceMatrix(ret, a, b, Braycurtis<typename A::Type>);
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

inline void BuildDistanceMatrixDistance(std::string const &custom_name, pybind11::module &module)
{
  using namespace fetch::math;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperDistanceMatrix<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperDistanceMatrix<RectangularArray<float>>);
  //    .def(custom_name.c_str(), &WrapperDistanceMatrix< ShapelessArray< double
  //    > >) .def(custom_name.c_str(), &WrapperDistanceMatrix< ShapelessArray<
  //    float > >);
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
