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

#include "math/tensor.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;

using SizeType  = fetch::math::SizeType;
using ArrayType = fetch::math::Tensor<double>;

template <typename T>
class TensorDataloaderTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types < fetch::math::Tensor<int>, fetch::math::Tensor<float>,
      fetch::math::Tensor<double>,
      fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>,
                          fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(TensorDataloaderTest, MyTypes);

TYPED_TEST(TensorDataloaderTest, serialize_tensor_dataloader)
{
  TypeParam label_tensor({1, 4});
  TypeParam data1_tensor({2, 3, 4});
  TypeParam data2_tensor({1, 3, 4});

  // generate a plausible tensor data loader
  auto t = fetch::ml::dataloaders::TensorDataLoader<TypeParam, TypeParam>(
      label_tensor.shape(), {data1_tensor.shape(), data2_tensor.shape()};);

  fetch::serializers::ByteArrayBuffer b;
  b << t;
  b.seek(0);
  fetch::ml::dataloaders::TensorDataLoader<TypeParam, TypeParam> t2;
  b >> t2;
  EXPECT_EQ(t, t2);
}
