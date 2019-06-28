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

#include "python/fetch_pybind.hpp"

#include "python/math/fixed_point/py_fixed_point.hpp"

#include "python/memory/py_array.hpp"
#include "python/memory/py_range.hpp"
#include "python/memory/py_shared_array.hpp"

#include "python/math/distance/py_braycurtis.hpp"
#include "python/math/distance/py_chebyshev.hpp"
#include "python/math/distance/py_cosine.hpp"
#include "python/math/distance/py_euclidean.hpp"
#include "python/math/distance/py_hamming.hpp"
#include "python/math/distance/py_jaccard.hpp"
#include "python/math/distance/py_manhattan.hpp"
#include "python/math/distance/py_pearson.hpp"

#include "python/math/py_tensor.hpp"
#include "python/math/spline/py_linear.hpp"

#include "python/math/correlation/py_cosine.hpp"
#include "python/math/correlation/py_jaccard.hpp"
#include "python/math/correlation/py_pearson.hpp"

#include "python/math/statistics/py_geometric_mean.hpp"
#include "python/math/statistics/py_max.hpp"
#include "python/math/statistics/py_mean.hpp"
#include "python/math/statistics/py_min.hpp"
#include "python/math/statistics/py_standard_deviation.hpp"
#include "python/math/statistics/py_variance.hpp"

#include "python/byte_array/py_encoders.hpp"
#include "python/byte_array/py_referenced_byte_array.hpp"

#include "python/byte_array/details/py_encode_decode.hpp"
#include "python/byte_array/py_basic_byte_array.hpp"
#include "python/byte_array/py_consumers.hpp"
#include "python/byte_array/py_decoders.hpp"
#include "python/byte_array/tokenizer/py_token.hpp"
#include "python/byte_array/tokenizer/py_tokenizer.hpp"

#include "python/random/py_lcg.hpp"
#include "python/random/py_lfg.hpp"

#include "python/ml/all.hpp"

#include "python/auctions/py_bid.hpp"
#include "python/auctions/py_combinatorial_auction.hpp"
#include "python/auctions/py_item.hpp"
#include "python/auctions/py_mock_smart_ledger.hpp"

#include "python/serializers/py_byte_array_buffer.hpp"

#include <cstdint>

namespace py = pybind11;

PYBIND11_MODULE(fetch, module)
{

  // Namespaces
  py::module ns_fetch_fixed_point      = module.def_submodule("fixed_point");
  py::module ns_fetch_random           = module.def_submodule("random");
  py::module ns_fetch_vectorize        = module.def_submodule("vectorize");
  py::module ns_fetch_image            = module.def_submodule("image");
  py::module ns_fetch_image_colors     = ns_fetch_image.def_submodule("colors");
  py::module ns_fetch_math             = module.def_submodule("math");
  py::module ns_fetch_ml               = module.def_submodule("ml");
  py::module ns_fetch_math_correlation = ns_fetch_math.def_submodule("correlation");
  py::module ns_fetch_math_distance    = ns_fetch_math.def_submodule("distance");
  py::module ns_fetch_math_clustering  = ns_fetch_math.def_submodule("clustering");
  py::module ns_fetch_math_statistics  = ns_fetch_math.def_submodule("statistics");
  py::module ns_fetch_math_spline      = ns_fetch_math.def_submodule("spline");
  py::module ns_fetch_math_tensor      = ns_fetch_math.def_submodule("tensor");
  py::module ns_fetch_memory           = module.def_submodule("memory");
  py::module ns_fetch_byte_array       = module.def_submodule("byte_array");
  py::module ns_fetch_math_linalg      = ns_fetch_math.def_submodule("linalg");
  py::module ns_fetch_auctions         = module.def_submodule("auctions");
  py::module ns_fetch_serializer       = module.def_submodule("serializers");

  fetch::math::BuildTensor<float>("TensorFloat", ns_fetch_math_tensor);
  fetch::math::BuildTensor<double>("TensorDouble", ns_fetch_math_tensor);
  fetch::math::BuildTensor<fetch::fixed_point::FixedPoint<32, 32>>("TensorFixed32_32",
                                                                   ns_fetch_math_tensor);

  fetch::memory::BuildArray<int8_t>("ArrayInt8", ns_fetch_memory);
  fetch::memory::BuildArray<int16_t>("ArrayInt16", ns_fetch_memory);
  fetch::memory::BuildArray<int32_t>("ArrayInt32", ns_fetch_memory);
  fetch::memory::BuildArray<int64_t>("ArrayInt64", ns_fetch_memory);

  fetch::memory::BuildArray<uint8_t>("ArrayUInt8", ns_fetch_memory);
  fetch::memory::BuildArray<uint16_t>("ArrayUInt16", ns_fetch_memory);
  fetch::memory::BuildArray<uint32_t>("ArrayUInt32", ns_fetch_memory);
  fetch::memory::BuildArray<uint64_t>("ArrayUInt64", ns_fetch_memory);

  fetch::memory::BuildArray<float>("ArrayFloat", ns_fetch_memory);
  fetch::memory::BuildArray<double>("ArrayDouble", ns_fetch_memory);

  fetch::memory::BuildSharedArray<int8_t>("SharedArrayInt8", ns_fetch_memory);
  fetch::memory::BuildSharedArray<int16_t>("SharedArrayInt16", ns_fetch_memory);
  fetch::memory::BuildSharedArray<int32_t>("SharedArrayInt32", ns_fetch_memory);
  fetch::memory::BuildSharedArray<int64_t>("SharedArrayInt64", ns_fetch_memory);

  fetch::memory::BuildSharedArray<uint8_t>("SharedArrayUInt8", ns_fetch_memory);
  fetch::memory::BuildSharedArray<uint16_t>("SharedArrayUInt16", ns_fetch_memory);
  fetch::memory::BuildSharedArray<uint32_t>("SharedArrayUInt32", ns_fetch_memory);
  fetch::memory::BuildSharedArray<uint64_t>("SharedArrayUInt64", ns_fetch_memory);

  fetch::memory::BuildSharedArray<float>("SharedArrayFloat", ns_fetch_memory);
  fetch::memory::BuildSharedArray<double>("SharedArrayDouble", ns_fetch_memory);

  fetch::memory::BuildRange("Range", ns_fetch_memory);

  ///////////
  // Comparisons

  ////////////

  // fetch::math::clustering::BuildKMeansClustering("KMeans", ns_fetch_math_clustering);

  ////////////
  fetch::byte_array::BuildConstByteArray(ns_fetch_byte_array);
  fetch::byte_array::BuildByteArray(ns_fetch_byte_array);

  fetch::random::BuildLaggedFibonacciGenerator<418, 1279>("LaggedFibonacciGenerator",
                                                          ns_fetch_random);
  fetch::random::BuildLinearCongruentialGenerator(ns_fetch_random);

  // py::module ns_fetch_network_swarm = module.def_submodule("network_swarm");
  // fetch::swarm::BuildSwarmAgentApi(ns_fetch_network_swarm);

  // FixedPoint
  fetch::fixed_point::BuildFixedPoint<32, 32>("FixedPoint32_32", ns_fetch_fixed_point);

  // Machine Learning
  py::module ns_fetch_ml_float = ns_fetch_ml.def_submodule("float");
  fetch::ml::BuildMLLibrary<float>(ns_fetch_ml_float);

  py::module ns_fetch_ml_double = ns_fetch_ml.def_submodule("double");
  fetch::ml::BuildMLLibrary<double>(ns_fetch_ml_double);

  py::module ns_fetch_ml_fixed_32_32 = ns_fetch_ml.def_submodule("fixed_32_32");
  fetch::ml::BuildMLLibrary<fetch::fixed_point::FixedPoint<32, 32>>(ns_fetch_ml_fixed_32_32);

  /////////////

  fetch::auctions::BuildCombinatorialAuction("CombinatorialAuction", ns_fetch_auctions);
  fetch::auctions::BuildItem("Item", ns_fetch_auctions);
  fetch::auctions::BuildBid("Bid", ns_fetch_auctions);

  fetch::auctions::BuildMockSmartLedger("MockSmartLedger", ns_fetch_auctions);

  fetch::serializers::BuildByteArrayBuffer(ns_fetch_serializer);
}
