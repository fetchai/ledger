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
#include "math/base_types.hpp"
#include "math/metrics/kl_divergence.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class KlDivergenceTest : public ::testing::Test
{
};

TYPED_TEST_CASE(KlDivergenceTest, TensorFloatingTypes);

TYPED_TEST(KlDivergenceTest, same_tensors_divergence_test)
{
  using DataType = typename TypeParam::Type;
  TypeParam A({4, 4});

  A.Set(SizeType{0}, SizeType{0}, fetch::math::Type<DataType>("0.1"));
  A.Set(SizeType{0}, SizeType{1}, fetch::math::Type<DataType>("0.2"));
  A.Set(SizeType{0}, SizeType{2}, fetch::math::Type<DataType>("0.3"));
  A.Set(SizeType{0}, SizeType{3}, fetch::math::Type<DataType>("0.4"));

  A.Set(SizeType{1}, SizeType{0}, fetch::math::Type<DataType>("-0.1"));
  A.Set(SizeType{1}, SizeType{1}, fetch::math::Type<DataType>("-0.2"));
  A.Set(SizeType{1}, SizeType{2}, fetch::math::Type<DataType>("-0.3"));
  A.Set(SizeType{1}, SizeType{3}, fetch::math::Type<DataType>("-0.4"));

  A.Set(SizeType{2}, SizeType{0}, fetch::math::Type<DataType>("-1.1"));
  A.Set(SizeType{2}, SizeType{1}, fetch::math::Type<DataType>("-1.2"));
  A.Set(SizeType{2}, SizeType{2}, fetch::math::Type<DataType>("-1.3"));
  A.Set(SizeType{2}, SizeType{3}, fetch::math::Type<DataType>("-1.4"));

  A.Set(SizeType{3}, SizeType{0}, fetch::math::Type<DataType>("1.1"));
  A.Set(SizeType{3}, SizeType{1}, fetch::math::Type<DataType>("1.2"));
  A.Set(SizeType{3}, SizeType{2}, fetch::math::Type<DataType>("1.3"));
  A.Set(SizeType{3}, SizeType{3}, fetch::math::Type<DataType>("1.4"));

  TypeParam B = A.Copy();

  EXPECT_NEAR(double(KlDivergence(A, B)), 0.0, 1e-5);
  EXPECT_NEAR(double(KlDivergence(B, A)), 0.0, 1e-5);
}

TYPED_TEST(KlDivergenceTest, other_divergence_test)
{
  using DataType = typename TypeParam::Type;
  TypeParam A({4, 4});

  A.Set(SizeType{0}, SizeType{0}, fetch::math::Type<DataType>("0.15"));
  A.Set(SizeType{0}, SizeType{1}, fetch::math::Type<DataType>("0.16"));
  A.Set(SizeType{0}, SizeType{2}, fetch::math::Type<DataType>("0.17"));
  A.Set(SizeType{0}, SizeType{3}, fetch::math::Type<DataType>("0.18"));

  A.Set(SizeType{1}, SizeType{0}, fetch::math::Type<DataType>("0.19"));
  A.Set(SizeType{1}, SizeType{1}, fetch::math::Type<DataType>("0.20"));
  A.Set(SizeType{1}, SizeType{2}, fetch::math::Type<DataType>("0.21"));
  A.Set(SizeType{1}, SizeType{3}, fetch::math::Type<DataType>("0.22"));

  A.Set(SizeType{2}, SizeType{0}, fetch::math::Type<DataType>("0.23"));
  A.Set(SizeType{2}, SizeType{1}, fetch::math::Type<DataType>("0.24"));
  A.Set(SizeType{2}, SizeType{2}, fetch::math::Type<DataType>("0.25"));
  A.Set(SizeType{2}, SizeType{3}, fetch::math::Type<DataType>("0.26"));

  A.Set(SizeType{3}, SizeType{0}, fetch::math::Type<DataType>("0.27"));
  A.Set(SizeType{3}, SizeType{1}, fetch::math::Type<DataType>("0.28"));
  A.Set(SizeType{3}, SizeType{2}, fetch::math::Type<DataType>("0.29"));
  A.Set(SizeType{3}, SizeType{3}, fetch::math::Type<DataType>("0.30"));

  TypeParam B({4, 4});

  B.Set(SizeType{0}, SizeType{0}, fetch::math::Type<DataType>("0.31"));
  B.Set(SizeType{0}, SizeType{1}, fetch::math::Type<DataType>("0.32"));
  B.Set(SizeType{0}, SizeType{2}, fetch::math::Type<DataType>("0.33"));
  B.Set(SizeType{0}, SizeType{3}, fetch::math::Type<DataType>("0.34"));

  B.Set(SizeType{1}, SizeType{0}, fetch::math::Type<DataType>("0.35"));
  B.Set(SizeType{1}, SizeType{1}, fetch::math::Type<DataType>("0.36"));
  B.Set(SizeType{1}, SizeType{2}, fetch::math::Type<DataType>("0.37"));
  B.Set(SizeType{1}, SizeType{3}, fetch::math::Type<DataType>("0.38"));

  B.Set(SizeType{2}, SizeType{0}, fetch::math::Type<DataType>("0.39"));
  B.Set(SizeType{2}, SizeType{1}, fetch::math::Type<DataType>("0.40"));
  B.Set(SizeType{2}, SizeType{2}, fetch::math::Type<DataType>("0.41"));
  B.Set(SizeType{2}, SizeType{3}, fetch::math::Type<DataType>("0.42"));

  B.Set(SizeType{3}, SizeType{0}, fetch::math::Type<DataType>("0.43"));
  B.Set(SizeType{3}, SizeType{1}, fetch::math::Type<DataType>("0.44"));
  B.Set(SizeType{3}, SizeType{2}, fetch::math::Type<DataType>("0.45"));
  B.Set(SizeType{3}, SizeType{3}, fetch::math::Type<DataType>("0.46"));

  EXPECT_NEAR(static_cast<double>(KlDivergence(A, B)), static_cast<double>(-1.920114985949124),
              10 * static_cast<double>(function_tolerance<DataType>()));
  EXPECT_NEAR(static_cast<double>(KlDivergence(B, A)), static_cast<double>(3.3324871063232422),
              10 * static_cast<double>(function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
