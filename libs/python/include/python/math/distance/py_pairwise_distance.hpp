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
#include "math/distance/pairwise_distance.hpp"
#include "math/linalg/matrix.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace distance {

template <typename A>
inline A WrapperPairWiseDistance(A const &a, std::string const &method = "eucludian")
{

  A ret;
  ret.Resize(1, a.height() * (a.height() - 1) / 2);
  if (method == "euclidean")
  {
    PairWiseDistance(ret, a, Euclidean<typename A::Type>);
  }
  else if (method == "hamming")
  {
    PairWiseDistance(ret, a, Hamming<typename A::Type>);
  }
  else if (method == "manhattan")
  {
    PairWiseDistance(ret, a, Manhattan<typename A::Type>);
  }
  else if (method == "pearson")
  {
    PairWiseDistance(ret, a, Pearson<typename A::Type>);
  }
  else if (method == "eisen")
  {
    PairWiseDistance(ret, a, Eisen<typename A::Type>);
  }
  else if (method == "cosine")
  {
    PairWiseDistance(ret, a, Eisen<typename A::Type>);
  }
  else if (method == "jaccard")
  {
    PairWiseDistance(ret, a, Jaccard<typename A::Type>);
  }
  else if (method == "genelralised jaccard")
  {
    PairWiseDistance(ret, a, GeneralisedJaccard<typename A::Type>);
  }
  else if (method == "braycurtis")
  {
    PairWiseDistance(ret, a, Braycurtis<typename A::Type>);
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

inline void BuildPairWiseDistanceDistance(std::string const &custom_name, pybind11::module &module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperPairWiseDistance<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperPairWiseDistance<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperPairWiseDistance<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperPairWiseDistance<RectangularArray<float>>);
  //    .def(custom_name.c_str(), &WrapperPairWiseDistance< ShapelessArray<
  //    double > >) .def(custom_name.c_str(), &WrapperPairWiseDistance<
  //    ShapelessArray< float > >);
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
