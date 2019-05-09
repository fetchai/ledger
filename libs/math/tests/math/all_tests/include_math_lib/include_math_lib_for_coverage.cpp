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

#include "math/arithmetic/comparison.hpp"

#include "math/clustering/k_means.hpp"
#include "math/clustering/knn.hpp"

#include "math/correlation/cosine.hpp"
#include "math/correlation/jaccard.hpp"
#include "math/correlation/pearson.hpp"

#include "math/distance/braycurtis.hpp"
#include "math/distance/chebyshev.hpp"
#include "math/distance/cosine.hpp"
#include "math/distance/euclidean.hpp"
#include "math/distance/hamming.hpp"
#include "math/distance/jaccard.hpp"
#include "math/distance/manhattan.hpp"
#include "math/distance/minkowski.hpp"
#include "math/distance/pairwise_distance.hpp"
#include "math/distance/pearson.hpp"

#include "math/kernels/L2Loss.hpp"

#include "math/meta/math_type_traits.hpp"

#include "math/ml/activation_functions/elu.hpp"
#include "math/ml/activation_functions/leaky_relu.hpp"
#include "math/ml/activation_functions/relu.hpp"
#include "math/ml/activation_functions/sigmoid.hpp"
#include "math/ml/activation_functions/softmax.hpp"

#include "math/ml/loss_functions/cross_entropy.hpp"
#include "math/ml/loss_functions/kl_divergence.hpp"
#include "math/ml/loss_functions/l2_loss.hpp"
#include "math/ml/loss_functions/l2_norm.hpp"
#include "math/ml/loss_functions/mean_square_error.hpp"

#include "math/spline/linear.hpp"

#include "math/standard_functions/abs.hpp"
#include "math/standard_functions/clamp.hpp"
#include "math/standard_functions/exp.hpp"
#include "math/standard_functions/fmod.hpp"
#include "math/standard_functions/log.hpp"
#include "math/standard_functions/pow.hpp"
#include "math/standard_functions/remainder.hpp"
#include "math/standard_functions/sqrt.hpp"

#include "math/statistics/entropy.hpp"
#include "math/statistics/geometric_mean.hpp"
#include "math/statistics/mean.hpp"
#include "math/statistics/normal.hpp"
#include "math/statistics/perplexity.hpp"
#include "math/statistics/standard_deviation.hpp"
#include "math/statistics/variance.hpp"

#include "math/approx_exp.hpp"
#include "math/base_types.hpp"
#include "math/bignumber.hpp"
#include "math/combinatorics.hpp"
#include "math/comparison.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/normalize_array.hpp"
#include "math/sign.hpp"
#include "math/tensor.hpp"
#include "math/tensor_broadcast.hpp"
#include "math/tensor_iterator.hpp"
#include "math/tensor_squeeze.hpp"
#include "math/trigonometry.hpp"
#include "math/type.hpp"

#include <gtest/gtest.h>

// This dummy test includes everything in the math library to ensure
// that coverage reports also show functionality that is completely
// untested
TEST(dummy_test, dummy_test)
{}