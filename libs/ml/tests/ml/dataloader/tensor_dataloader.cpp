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

#include "ml/dataloaders/tensor_dataloader.hpp"

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <gtest/gtest.h>

template <typename T>
class TensorDataloaderTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(TensorDataloaderTest, MyTypes);

TYPED_TEST(TensorDataloaderTest, empty_loader_test)
{
  fetch::ml::TensorDataLoader<uint64_t, std::shared_ptr<TypeParam>> loader;
  EXPECT_EQ(loader.Size(), 0);
  EXPECT_TRUE(loader.IsDone());
  EXPECT_THROW(loader.GetNext(), std::out_of_range);
}

TYPED_TEST(TensorDataloaderTest, loader_test)
{
  fetch::ml::TensorDataLoader<int64_t, std::shared_ptr<TypeParam>> loader;

  // Fill with data
  for (int64_t i(-10); i < 10; ++i)
  {
    std::shared_ptr<TypeParam> t =
        std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
    t->Fill(typename TypeParam::Type(i));

    std::vector<std::shared_ptr<TypeParam>> t_vec;
    t_vec.emplace_back(t);

    loader.Add(std::pair<int64_t, std::vector<std::shared_ptr<TypeParam>>>(i, t_vec));
  }
  // Consume all data
  for (int64_t i(-10); i < 10; ++i)
  {
    EXPECT_EQ(loader.Size(), 20);
    EXPECT_FALSE(loader.IsDone());

    auto cur_tensor = loader.GetNext();
    for (int64_t j(0); j < 5; j++)
    {
      for (int64_t k(0); k < 5; ++k)
      {
        EXPECT_EQ(cur_tensor.second[0]->At(j, k), typename TypeParam::Type(i));
      }
    }
  }
  EXPECT_TRUE(loader.IsDone());
  EXPECT_THROW(loader.GetNext(), std::out_of_range);
  // Reset
  loader.Reset();
  // Consume all data again
  for (int64_t i(-10); i < 10; ++i)
  {
    EXPECT_EQ(loader.Size(), 20);
    EXPECT_FALSE(loader.IsDone());
    EXPECT_EQ(loader.GetNext().first, i);
  }
  EXPECT_TRUE(loader.IsDone());
}
