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

#include "core/serializers/byte_array_buffer.hpp"
#include "math/tensor.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/serializers/ml_types.hpp"
#include "vectorise/fixed_point/serializers.hpp"

#include "gtest/gtest.h"

template <typename T>
class SerializersTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int32_t>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(SerializersTest, MyTypes);

TYPED_TEST(SerializersTest, serialize_empty_state_dict)
{
  fetch::ml::StateDict<TypeParam>     sd1;
  fetch::serializers::ByteArrayBuffer b;
  b << sd1;
  b.seek(0);
  fetch::ml::StateDict<TypeParam> sd2;
  b >> sd2;
  EXPECT_EQ(sd1, sd2);
}

TYPED_TEST(SerializersTest, serialize_state_dict)
{
  // Generate a plausible state dict out of a fully connected layer
  fetch::ml::layers::FullyConnected<TypeParam> fc(10, 10);
  struct fetch::ml::StateDict<TypeParam>       sd1 = fc.StateDict();
  fetch::serializers::ByteArrayBuffer          b;
  b << sd1;
  b.seek(0);
  fetch::ml::StateDict<TypeParam> sd2;
  b >> sd2;
  EXPECT_EQ(sd1, sd2);
}
