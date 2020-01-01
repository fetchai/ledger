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
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class TensorConstructorTest : public ::testing::Test
{
};

TYPED_TEST_CASE(TensorConstructorTest, IntAndFloatingTypes);

TYPED_TEST(TensorConstructorTest, string_construction)
{
  auto tensor = fetch::math::Tensor<TypeParam>::FromString(R"(1 3 4)");
  EXPECT_EQ(tensor.At(0, 0), TypeParam(1));
  EXPECT_EQ(tensor.At(0, 1), TypeParam(3));
  EXPECT_EQ(tensor.At(0, 2), TypeParam(4));

  auto tensor1 = fetch::math::Tensor<TypeParam>::FromString(R"(
  	1 -3 4 4.1
  	5 6 -7 7.999
  	8 9 -10 11.11111
  )");
  EXPECT_EQ(tensor1.shape().at(0), 3);
  EXPECT_EQ(tensor1.shape().at(1), 4);
  EXPECT_EQ(tensor1.At(0, 0), math::Type<TypeParam>("1"));
  EXPECT_EQ(tensor1.At(0, 1), math::Type<TypeParam>("-3"));
  EXPECT_EQ(tensor1.At(0, 2), math::Type<TypeParam>("4"));
  EXPECT_EQ(tensor1.At(0, 3), math::Type<TypeParam>("4.1"));
  EXPECT_EQ(tensor1.At(1, 0), math::Type<TypeParam>("5"));
  EXPECT_EQ(tensor1.At(1, 1), math::Type<TypeParam>("6"));
  EXPECT_EQ(tensor1.At(1, 2), math::Type<TypeParam>("-7"));
  EXPECT_EQ(tensor1.At(1, 3), math::Type<TypeParam>("7.999"));
  EXPECT_EQ(tensor1.At(2, 0), math::Type<TypeParam>("8"));
  EXPECT_EQ(tensor1.At(2, 1), math::Type<TypeParam>("9"));
  EXPECT_EQ(tensor1.At(2, 2), math::Type<TypeParam>("-10"));
  EXPECT_EQ(tensor1.At(2, 3), math::Type<TypeParam>("11.11111"));

  auto tensor2 = fetch::math::Tensor<TypeParam>::FromString(R"(
  	1 -3 4 4.1;
  	5 6 -7 +7.999;
  	8 9 -10 +11.11111
  )");
  EXPECT_EQ(tensor1, tensor2);

  auto tensor3 = fetch::math::Tensor<TypeParam>::FromString(R"(
  	1 -3 4 4.1; 5 6 -7 7.999; 8 9 -10 11.11111
  )");
  EXPECT_EQ(tensor1, tensor3);

  auto tensor4 = fetch::math::Tensor<TypeParam>::FromString(R"(
  	+1 -3 +4 +4.1; 5 +6 -7 +7.999; 8 +9 -10 +11.11111
  )");
  EXPECT_EQ(tensor1, tensor4);
}

TYPED_TEST(TensorConstructorTest, string_construction_bad_formatting)
{
  auto tensor = fetch::math::Tensor<TypeParam>::FromString(R"(


  	1 3 4 4.1
  	;

  	5 6 7 7.999
  	;;;

  	8 9 10 11.11111;

  	)");
  EXPECT_EQ(tensor.shape().at(0), 3);
  EXPECT_EQ(tensor.shape().at(1), 4);
  EXPECT_EQ(tensor.At(0, 0), math::Type<TypeParam>("1"));
  EXPECT_EQ(tensor.At(0, 1), math::Type<TypeParam>("3"));
  EXPECT_EQ(tensor.At(0, 2), math::Type<TypeParam>("4"));
  EXPECT_EQ(tensor.At(0, 3), math::Type<TypeParam>("4.1"));
  EXPECT_EQ(tensor.At(1, 0), math::Type<TypeParam>("5"));
  EXPECT_EQ(tensor.At(1, 1), math::Type<TypeParam>("6"));
  EXPECT_EQ(tensor.At(1, 2), math::Type<TypeParam>("7"));
  EXPECT_EQ(tensor.At(1, 3), math::Type<TypeParam>("7.999"));
  EXPECT_EQ(tensor.At(2, 0), math::Type<TypeParam>("8"));
  EXPECT_EQ(tensor.At(2, 1), math::Type<TypeParam>("9"));
  EXPECT_EQ(tensor.At(2, 2), math::Type<TypeParam>("10"));
  EXPECT_EQ(tensor.At(2, 3), math::Type<TypeParam>("11.11111"));
}

TYPED_TEST(TensorConstructorTest, string_construction_bad_formatting_2)
{
  auto tensor = fetch::math::Tensor<TypeParam>::FromString(R"(


  	1 3 4 4.1
  	;

  	5 6 7 7.999
  	;;; \r

  	8 9 10 11.11111;

  	)");
  EXPECT_EQ(tensor.shape().at(0), 3);
  EXPECT_EQ(tensor.shape().at(1), 4);
  EXPECT_EQ(tensor.At(0, 0), math::Type<TypeParam>("1"));
  EXPECT_EQ(tensor.At(0, 1), math::Type<TypeParam>("3"));
  EXPECT_EQ(tensor.At(0, 2), math::Type<TypeParam>("4"));
  EXPECT_EQ(tensor.At(0, 3), math::Type<TypeParam>("4.1"));
  EXPECT_EQ(tensor.At(1, 0), math::Type<TypeParam>("5"));
  EXPECT_EQ(tensor.At(1, 1), math::Type<TypeParam>("6"));
  EXPECT_EQ(tensor.At(1, 2), math::Type<TypeParam>("7"));
  EXPECT_EQ(tensor.At(1, 3), math::Type<TypeParam>("7.999"));
  EXPECT_EQ(tensor.At(2, 0), math::Type<TypeParam>("8"));
  EXPECT_EQ(tensor.At(2, 1), math::Type<TypeParam>("9"));
  EXPECT_EQ(tensor.At(2, 2), math::Type<TypeParam>("10"));
  EXPECT_EQ(tensor.At(2, 3), math::Type<TypeParam>("11.11111"));
}

TYPED_TEST(TensorConstructorTest, string_construction_bad_formatting_3)
{
  auto tensor = fetch::math::Tensor<TypeParam>::FromString(R"(
  	1 3 4 4.1 \r 5 6 7 7.999 \r
  	8 9 10 11.11111
  	)");
  EXPECT_EQ(tensor.shape().at(0), 3);
  EXPECT_EQ(tensor.shape().at(1), 4);
  EXPECT_EQ(tensor.At(0, 0), math::Type<TypeParam>("1"));
  EXPECT_EQ(tensor.At(0, 1), math::Type<TypeParam>("3"));
  EXPECT_EQ(tensor.At(0, 2), math::Type<TypeParam>("4"));
  EXPECT_EQ(tensor.At(0, 3), math::Type<TypeParam>("4.1"));
  EXPECT_EQ(tensor.At(1, 0), math::Type<TypeParam>("5"));
  EXPECT_EQ(tensor.At(1, 1), math::Type<TypeParam>("6"));
  EXPECT_EQ(tensor.At(1, 2), math::Type<TypeParam>("7"));
  EXPECT_EQ(tensor.At(1, 3), math::Type<TypeParam>("7.999"));
  EXPECT_EQ(tensor.At(2, 0), math::Type<TypeParam>("8"));
  EXPECT_EQ(tensor.At(2, 1), math::Type<TypeParam>("9"));
  EXPECT_EQ(tensor.At(2, 2), math::Type<TypeParam>("10"));
  EXPECT_EQ(tensor.At(2, 3), math::Type<TypeParam>("11.11111"));
}

TYPED_TEST(TensorConstructorTest, string_construction_invalid_formatting)
{
  EXPECT_THROW(fetch::math::Tensor<TypeParam>::FromString(R"(
  	1 3 4 4.1;
  	5 6 7 7.999
  	8 9 10 11.11111;
  	)"),
               fetch::math::exceptions::WrongShape);

  EXPECT_THROW(fetch::math::Tensor<TypeParam>::FromString(R"(
  	1 3 4 4.1
  	5 6 7 7.999; 8 9 10 11.11111
  	)"),
               fetch::math::exceptions::WrongShape);

  EXPECT_THROW(fetch::math::Tensor<TypeParam>::FromString(R"(
  	1 3 4 4.1;
  	;
  	5 6 7 7.999
  	8 9 10 11.11111
  	)"),
               fetch::math::exceptions::WrongShape);

  EXPECT_THROW(fetch::math::Tensor<TypeParam>::FromString(R"(
  	1 3 4 4.1;
  	5 6 7;
  	8 9 10 11.11111;
  	)"),
               fetch::math::exceptions::WrongShape);

  EXPECT_THROW(fetch::math::Tensor<TypeParam>::FromString(R"(
  	1 3 4 4.1
  	5 6 7
  	8 9 10 11.11111
  	)"),
               fetch::math::exceptions::WrongShape);
}

}  // namespace test
}  // namespace math
}  // namespace fetch
