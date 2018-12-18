//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
#include "python/math/py_shapeless_array.hpp"
#include "python/memory/py_array.hpp"
#include "python/memory/py_range.hpp"
#include "python/memory/py_rectangular_array.hpp"
#include "python/memory/py_shared_array.hpp"

#include "python/math/distance/py_braycurtis.hpp"
#include "python/math/distance/py_chebyshev.hpp"
#include "python/math/distance/py_distance_matrix.hpp"
#include "python/math/distance/py_eisen.hpp"
#include "python/math/distance/py_euclidean.hpp"
#include "python/math/distance/py_hamming.hpp"
#include "python/math/distance/py_jaccard.hpp"
#include "python/math/distance/py_manhattan.hpp"
#include "python/math/distance/py_pairwise_distance.hpp"
#include "python/math/distance/py_pearson.hpp"

#include "python/math/clustering/py_kmeans.hpp"

#include "python/math/linalg/py_matrix.hpp"
#include "python/math/py_bignumber.hpp"
#include "python/math/py_exp.hpp"
#include "python/math/py_log.hpp"
#include "python/math/py_ndarray.hpp"
#include "python/math/spline/py_linear.hpp"

#include "python/math/correlation/py_eisen.hpp"
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

#include "python/ml/layers/py_layer.hpp"
#include "python/ml/ops/py_ops.hpp"
#include "python/ml/py_session.hpp"
#include "python/ml/py_variable.hpp"

// !!!!
namespace py = pybind11;

//#include "python/network/swarm/py_swarm_agent_api.hpp"

PYBIND11_MODULE(fetch, module)
{

  // Namespaces

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
  py::module ns_fetch_memory           = module.def_submodule("memory");
  py::module ns_fetch_byte_array       = module.def_submodule("byte_array");
  py::module ns_fetch_math_linalg      = ns_fetch_math.def_submodule("linalg");

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
  /*
  fetch::math::BuildShapelessArray<int8_t>("ShapelessArrayInt8",
  ns_fetch_memory);
  fetch::math::BuildShapelessArray<int16_t>("ShapelessArrayInt16",
  ns_fetch_memory);
  fetch::math::BuildShapelessArray<int32_t>("ShapelessArrayInt32",
  ns_fetch_memory);
  fetch::math::BuildShapelessArray<int64_t>("ShapelessArrayInt64",
  ns_fetch_memory);

  fetch::math::BuildShapelessArray<uint8_t>("ShapelessArrayUInt8",
  ns_fetch_memory);
  fetch::math::BuildShapelessArray<uint16_t>("ShapelessArrayUInt16",
  ns_fetch_memory);
  fetch::math::BuildShapelessArray<uint32_t>("ShapelessArrayUInt32",
  ns_fetch_memory);
  fetch::math::BuildShapelessArray<uint64_t>("ShapelessArrayUInt64",
  ns_fetch_memory);
  */
  fetch::math::BuildShapelessArray<float>("ShapelessArrayFloat", ns_fetch_memory);
  fetch::math::BuildShapelessArray<double>("ShapelessArrayDouble", ns_fetch_memory);

  /*
  fetch::math::BuildRectangularArray<int8_t>("RectangularArrayInt8",
  ns_fetch_memory);
  fetch::math::BuildRectangularArray<int16_t>("RectangularArrayInt16",
  ns_fetch_memory);
  fetch::math::BuildRectangularArray<int32_t>("RectangularArrayInt32",
  ns_fetch_memory);
  fetch::math::BuildRectangularArray<int64_t>("RectangularArrayInt64",
  ns_fetch_memory);

  fetch::math::BuildRectangularArray<uint8_t>("RectangularArrayUInt8",
  ns_fetch_memory);
  fetch::math::BuildRectangularArray<uint16_t>("RectangularArrayUInt16",
  ns_fetch_memory);
  fetch::math::BuildRectangularArray<uint32_t>("RectangularArrayUInt32",
  ns_fetch_memory);
  fetch::math::BuildRectangularArray<uint64_t>("RectangularArrayUInt64",
  ns_fetch_memory);
  */
  fetch::math::BuildRectangularArray<float>("RectangularArrayFloat", ns_fetch_memory);
  fetch::math::BuildRectangularArray<double>("RectangularArrayDouble", ns_fetch_memory);

  //  fetch::math::BuildExp< 0, 60801, false>("Exp0", ns_fetch_math);
  //  fetch::math::BuildLog(ns_fetch_math);

  //  fetch::math::linalg::BuildMatrix<int8_t>("MatrixInt8",
  //  ns_fetch_math_linalg);
  //  fetch::math::linalg::BuildMatrix<int16_t>("MatrixInt16",
  //  ns_fetch_math_linalg);
  //  fetch::math::linalg::BuildMatrix<int32_t>("MatrixInt32",
  //  ns_fetch_math_linalg);
  //  fetch::math::linalg::BuildMatrix<int64_t>("MatrixInt64",
  //  ns_fetch_math_linalg);

  //  fetch::math::linalg::BuildMatrix<uint8_t>("MatrixUInt8",
  //  ns_fetch_math_linalg);
  //  fetch::math::linalg::BuildMatrix<uint16_t>("MatrixUInt16",
  //  ns_fetch_math_linalg);
  //  fetch::math::linalg::BuildMatrix<uint32_t>("MatrixUInt32",
  //  ns_fetch_math_linalg);
  //  fetch::math::linalg::BuildMatrix<uint64_t>("MatrixUInt64",
  //  ns_fetch_math_linalg);

  fetch::math::linalg::BuildMatrix<float>("MatrixFloat", ns_fetch_math_linalg);
  fetch::math::linalg::BuildMatrix<double>("MatrixDouble", ns_fetch_math_linalg);

  fetch::math::BuildNDArray<float>("NDArrayFloat", ns_fetch_math);
  fetch::math::BuildNDArray<double>("NDArrayDouble", ns_fetch_math);
  //  fetch::math::BuildNDArray<std::size_t>("NDArrayUInt", ns_fetch_math);

  //  fetch::math::BuildSpline(ns_fetch_math_spline);
  //  fetch::image::colors::BuildAbstractColor<uint32_t, 8,
  //  3>("ColorRGB8",ns_fetch_image_colors);
  //  fetch::image::colors::BuildAbstractColor<uint32_t, 8,
  //  4>("ColorRGBA8",ns_fetch_image_colors);

  //  fetch::image::BuildImageType< fetch::image::colors::RGB8 >("ImageRGB8",
  //  ns_fetch_image); fetch::image::BuildImageType< fetch::image::colors::RGBA8
  //  >("ImageRGBA8", ns_fetch_image);

  ///////////
  // Comparisons

  fetch::math::correlation::BuildPearsonCorrelation("Pearson", ns_fetch_math_correlation);
  fetch::math::correlation::BuildEisenCorrelation("Eisen", ns_fetch_math_correlation);
  fetch::math::correlation::BuildJaccardCorrelation("Jaccard", ns_fetch_math_correlation);
  fetch::math::correlation::BuildGeneralisedJaccardCorrelation("GeneralisedJaccard",
                                                               ns_fetch_math_correlation);

  fetch::math::distance::BuildPearsonDistance("Pearson", ns_fetch_math_distance);
  fetch::math::distance::BuildEisenDistance("Eisen", ns_fetch_math_distance);
  fetch::math::distance::BuildEisenDistance("Cosine", ns_fetch_math_distance);
  fetch::math::distance::BuildManhattanDistance("Manhattan", ns_fetch_math_distance);
  fetch::math::distance::BuildEuclideanDistance("Euclidean", ns_fetch_math_distance);
  fetch::math::distance::BuildJaccardDistance("Jaccard", ns_fetch_math_distance);
  fetch::math::distance::BuildGeneralisedJaccardDistance("GeneralisedJaccard",
                                                         ns_fetch_math_distance);

  fetch::math::distance::BuildHammingDistance("Hamming", ns_fetch_math_distance);
  fetch::math::distance::BuildChebyshevDistance("Chebyshev", ns_fetch_math_distance);
  fetch::math::distance::BuildBraycurtisDistance("Braycurtis", ns_fetch_math_distance);

  fetch::math::distance::BuildDistanceMatrixDistance("DistanceMatrix", ns_fetch_math_distance);
  fetch::math::distance::BuildPairWiseDistanceDistance("PairWiseDistance", ns_fetch_math_distance);

  ////////////

  fetch::math::clustering::BuildKMeansClustering("KMeans", ns_fetch_math_clustering);

  ////////////

  fetch::math::BuildExpStatistics("Exp", ns_fetch_math_statistics);
  fetch::math::BuildLogStatistics("Log", ns_fetch_math_statistics);

  // Statisticsfetch_math_statistics);
  fetch::math::BuildMinStatistics("Min", ns_fetch_math_statistics);
  fetch::math::BuildMaxStatistics("Max", ns_fetch_math_statistics);
  fetch::math::statistics::BuildMeanStatistics("Mean", ns_fetch_math_statistics);
  fetch::math::statistics::BuildGeometricMeanStatistics("GeometricMean", ns_fetch_math_statistics);
  fetch::math::statistics::BuildVarianceStatistics("Variance", ns_fetch_math_statistics);
  fetch::math::statistics::BuildStandardDeviationStatistics("StandardDeviation",
                                                            ns_fetch_math_statistics);

  fetch::byte_array::BuildConstByteArray(ns_fetch_byte_array);
  fetch::byte_array::BuildByteArray(ns_fetch_byte_array);

  fetch::random::BuildLaggedFibonacciGenerator<418, 1279>("LaggedFibonacciGenerator",
                                                          ns_fetch_random);
  fetch::random::BuildLinearCongruentialGenerator(ns_fetch_random);

  // py::module ns_fetch_network_swarm = module.def_submodule("network_swarm");
  // fetch::swarm::BuildSwarmAgentApi(ns_fetch_network_swarm);

  // Machine Learning

  using ArrayType = fetch::math::linalg::Matrix<double>;
  fetch::ml::BuildVariable<ArrayType>("Variable", ns_fetch_ml);
  using VariableType = fetch::ml::Variable<fetch::math::linalg::Matrix<double>>;
  fetch::ml::BuildSession<ArrayType, VariableType>("Session", ns_fetch_ml);
  fetch::ml::layers::BuildLayers<ArrayType>("Layer", ns_fetch_ml);

  fetch::ml::ops::BuildOps<ArrayType>("Ops", ns_fetch_ml);
}
