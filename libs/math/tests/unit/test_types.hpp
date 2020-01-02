#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "gtest/gtest.h"
#include "math/tensor/tensor.hpp"

namespace fetch {
namespace math {
namespace test {

using FixedPointTypes = ::testing::Types<fetch::fixed_point::fp32_t, fetch::fixed_point::fp64_t,
                                         fetch::fixed_point::fp128_t>;

using FloatingTypes =
    ::testing::Types<float, double, fetch::fixed_point::fp32_t, fetch::fixed_point::fp64_t>;
using HighPrecisionFloatingTypes =
    ::testing::Types<float, double, fetch::fixed_point::fp64_t, fetch::fixed_point::fp128_t>;

using IntAndFloatingTypes =
    ::testing::Types<int32_t, int64_t, float, double, fetch::fixed_point::fp32_t,
                     fetch::fixed_point::fp64_t, fetch::fixed_point::fp128_t>;
using FloatIntAndUIntTypes =
    ::testing::Types<uint32_t, int32_t, uint64_t, int64_t, float, double,
                     fetch::fixed_point::fp32_t, fetch::fixed_point::fp64_t,
                     fetch::fixed_point::fp128_t>;

using TensorFloatingTypes =
    ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                     fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                     fetch::math::Tensor<fetch::fixed_point::fp64_t>,
                     fetch::math::Tensor<fetch::fixed_point::fp128_t>>;

using HighPrecisionTensorFloatingTypes =
    ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                     fetch::math::Tensor<fetch::fixed_point::fp64_t>>;

using TensorIntAndFloatingTypes =
    ::testing::Types<fetch::math::Tensor<int32_t>, fetch::math::Tensor<int64_t>,
                     fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                     fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                     fetch::math::Tensor<fetch::fixed_point::fp64_t>,
                     fetch::math::Tensor<fetch::fixed_point::fp128_t>>;

using TensorFloatIntAndUIntTypes =
    ::testing::Types<fetch::math::Tensor<int32_t>, fetch::math::Tensor<uint32_t>,
                     fetch::math::Tensor<int64_t>, fetch::math::Tensor<uint64_t>,
                     fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                     fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                     fetch::math::Tensor<fetch::fixed_point::fp64_t>,
                     fetch::math::Tensor<fetch::fixed_point::fp128_t>>;

using HighPrecisionTensorFixedPointTypes =
    ::testing::Types<fetch::math::Tensor<fetch::fixed_point::fp64_t>>;

using HighPrecisionTensorNoFixedPointFloatingTypes =
    ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>>;

}  // namespace test
}  // namespace math
}  // namespace fetch
