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

#include "core/serializers/main_serializer.hpp"
#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;

template <typename T>
class TensorDataloaderTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(TensorDataloaderTest, MyTypes);

TYPED_TEST(TensorDataloaderTest, serialize_tensor_dataloader)
{
  TypeParam label_tensor = TypeParam::UniformRandom(4);
  TypeParam data1_tensor = TypeParam::UniformRandom(24);
  label_tensor.Reshape({1, 4});
  data1_tensor.Reshape({2, 3, 4});

  // generate a plausible tensor data loader
  auto tdl = fetch::ml::dataloaders::TensorDataLoader<TypeParam, TypeParam>(label_tensor.shape(),
                                                                            {data1_tensor.shape()});

  // add some data
  tdl.AddData(data1_tensor, label_tensor);

  fetch::serializers::MsgPackSerializer b;
  b << tdl;

  b.seek(0);

  // initialise a new tensor dataloader with the wrong shape parameters
  // these will get updated by deserialisation
  fetch::ml::dataloaders::TensorDataLoader<TypeParam, TypeParam> tdl_2({1, 1}, {{1, 1}}, false,
                                                                       0.5);
  b >> tdl_2;

  EXPECT_EQ(tdl.Size(false), tdl_2.Size(false));
  EXPECT_EQ(tdl.Size(true), tdl_2.Size(true));
  EXPECT_EQ(tdl.IsDone(false), tdl_2.IsDone(false));
  EXPECT_EQ(tdl.IsDone(true), tdl_2.IsDone(true));
  EXPECT_EQ(tdl.GetNext(false), tdl_2.GetNext(false));
  EXPECT_EQ(tdl.GetNext(true), tdl_2.GetNext(true));

  // add some new data
  label_tensor = TypeParam::UniformRandom(4);
  data1_tensor = TypeParam::UniformRandom(24);
  label_tensor.Reshape({1, 4});
  data1_tensor.Reshape({2, 3, 4});
  EXPECT_EQ(tdl.AddData(data1_tensor, label_tensor), tdl_2.AddData(data1_tensor, label_tensor));

  EXPECT_EQ(tdl.Size(false), tdl_2.Size(false));
  EXPECT_EQ(tdl.Size(true), tdl_2.Size(true));
  EXPECT_EQ(tdl.IsDone(false), tdl_2.IsDone(false));
  EXPECT_EQ(tdl.IsDone(true), tdl_2.IsDone(true));
  EXPECT_EQ(tdl.GetNext(false), tdl_2.GetNext(false));
  EXPECT_EQ(tdl.GetNext(true), tdl_2.GetNext(true));
}
