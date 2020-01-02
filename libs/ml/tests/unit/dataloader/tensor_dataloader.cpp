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

#include "core/serializers/main_serializer.hpp"
#include "math/base_types.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class TensorDataloaderTest : public ::testing::Test
{
};

TYPED_TEST_CASE(TensorDataloaderTest, math::test::TensorFloatingTypes);

TYPED_TEST(TensorDataloaderTest, serialize_tensor_dataloader)
{
  TypeParam label_tensor = TypeParam::UniformRandomIntegers(4, 0, 100);
  TypeParam data1_tensor = TypeParam::UniformRandomIntegers(24, 0, 100);
  TypeParam data2_tensor = TypeParam::UniformRandomIntegers(32, 0, 100);
  label_tensor.Reshape({1, 4});
  data1_tensor.Reshape({2, 3, 4});
  data2_tensor.Reshape({8, 2, 4});

  // generate a plausible tensor data loader
  auto tdl = fetch::ml::dataloaders::TensorDataLoader<TypeParam, TypeParam>();

  // add some data
  tdl.AddData({data1_tensor, data2_tensor}, label_tensor);
  tdl.SetRandomMode(true);
  // calling GetNext ensures that internal parameters are not default
  auto next0 = tdl.GetNext();

  fetch::serializers::MsgPackSerializer b;
  b << tdl;

  b.seek(0);

  fetch::ml::dataloaders::TensorDataLoader<TypeParam, TypeParam> tdl_2;
  tdl_2.SetTestRatio(0.5);

  b >> tdl_2;

  EXPECT_EQ(tdl.Size(), tdl_2.Size());
  EXPECT_EQ(tdl.IsDone(), tdl_2.IsDone());

  auto next1 = tdl.GetNext();
  auto next2 = tdl_2.GetNext();

  EXPECT_TRUE(next1.first.AllClose(next2.first));
  EXPECT_TRUE(next1.second.at(0).AllClose(next2.second.at(0)));
  EXPECT_TRUE(next1.second.at(1).AllClose(next2.second.at(1)));

  // add some new data
  label_tensor = TypeParam::UniformRandom(4);
  data1_tensor = TypeParam::UniformRandom(24);
  data2_tensor = TypeParam::UniformRandom(32);
  label_tensor.Reshape({1, 4});
  data1_tensor.Reshape({2, 3, 4});
  data2_tensor.Reshape({8, 2, 4});
  EXPECT_EQ(tdl.AddData({data1_tensor, data2_tensor}, label_tensor),
            tdl_2.AddData({data1_tensor, data2_tensor}, label_tensor));

  EXPECT_EQ(tdl.Size(), tdl_2.Size());
  EXPECT_EQ(tdl.IsDone(), tdl_2.IsDone());

  next1 = tdl.GetNext();
  next2 = tdl_2.GetNext();
  EXPECT_TRUE(next1.first.AllClose(next2.first));
  EXPECT_TRUE(next1.second.at(0).AllClose(next2.second.at(0)));
  EXPECT_TRUE(next1.second.at(1).AllClose(next2.second.at(1)));
}

TYPED_TEST(TensorDataloaderTest, test_validation_splitting_dataloader_test)
{
  TypeParam label_tensor = TypeParam::UniformRandom(4);
  TypeParam data1_tensor = TypeParam::UniformRandom(24);
  TypeParam data2_tensor = TypeParam::UniformRandom(32);
  label_tensor.Reshape({1, 1});
  data1_tensor.Reshape({2, 3, 1});
  data2_tensor.Reshape({8, 2, 4});

  // generate a plausible tensor data loader
  auto tdl = fetch::ml::dataloaders::TensorDataLoader<TypeParam, TypeParam>();
  tdl.SetTestRatio(0.1f);
  tdl.SetValidationRatio(0.1f);

  // add some data
  tdl.AddData({data1_tensor, data2_tensor}, label_tensor);

  EXPECT_EQ(tdl.Size(), 1);
  EXPECT_THROW(tdl.SetMode(dataloaders::DataLoaderMode::TEST), std::runtime_error);
  EXPECT_THROW(tdl.SetMode(dataloaders::DataLoaderMode::VALIDATE), std::runtime_error);
}

TYPED_TEST(TensorDataloaderTest, prepare_batch_test)
{
  using SizeType = fetch::math::SizeType;

  SizeType feature_size_1_1 = 2;
  SizeType feature_size_1_2 = 3;
  SizeType feature_size_2_1 = 5;
  SizeType feature_size_2_2 = 4;
  SizeType batch_size       = 2;
  SizeType n_data           = 10;

  TypeParam label_tensor = TypeParam::UniformRandom(n_data);
  TypeParam data1_tensor = TypeParam::UniformRandom(feature_size_1_1 * feature_size_1_2 * n_data);
  TypeParam data2_tensor = TypeParam::UniformRandom(feature_size_2_1 * feature_size_2_2 * n_data);
  label_tensor.Reshape({1, n_data});
  data1_tensor.Reshape({feature_size_1_1, feature_size_1_2, n_data});
  data2_tensor.Reshape({feature_size_2_1, feature_size_2_2, n_data});

  // generate a plausible tensor data loader
  auto tdl = fetch::ml::dataloaders::TensorDataLoader<TypeParam, TypeParam>();
  // add some data
  tdl.AddData({data1_tensor, data2_tensor}, label_tensor);

  bool is_done_set = false;
  auto batch       = tdl.PrepareBatch(batch_size, is_done_set).second;

  EXPECT_EQ(batch.at(0).shape(),
            std::vector<SizeType>({feature_size_1_1, feature_size_1_2, batch_size}));
  EXPECT_EQ(batch.at(1).shape(),
            std::vector<SizeType>({feature_size_2_1, feature_size_2_2, batch_size}));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
