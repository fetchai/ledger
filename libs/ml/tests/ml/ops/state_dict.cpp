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

#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include "ml/ops/weights.hpp"
#include <gtest/gtest.h>

template <typename T>
class StateDictTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(StateDictTest, MyTypes);

TYPED_TEST(StateDictTest, empty_merge_test)
{
  fetch::ml::StateDict<TypeParam> a;
  fetch::ml::StateDict<TypeParam> b;

  EXPECT_FALSE(a.weights_);
  EXPECT_FALSE(b.weights_);
  EXPECT_TRUE(a.dict_.empty());
  EXPECT_TRUE(b.dict_.empty());
  a.Merge(b);
  EXPECT_FALSE(a.weights_);
  EXPECT_FALSE(b.weights_);
  EXPECT_TRUE(a.dict_.empty());
  EXPECT_TRUE(b.dict_.empty());
}

TYPED_TEST(StateDictTest, merge_test)
{
  fetch::ml::StateDict<TypeParam> a;
  fetch::ml::StateDict<TypeParam> b;
  a.weights_ = std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  b.weights_ = std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  a.weights_->Fill(typename TypeParam::Type(5));
  b.weights_->Fill(typename TypeParam::Type(3));
  for (unsigned int i(0); i < 25; ++i)
  {
    EXPECT_EQ(a.weights_->At(i), typename TypeParam::Type(5));
    EXPECT_EQ(b.weights_->At(i), typename TypeParam::Type(3));
  }
  a.Merge(b);
  for (unsigned int i(0); i < 25; ++i)
  {
    EXPECT_EQ(a.weights_->At(i), typename TypeParam::Type(4));
    EXPECT_EQ(b.weights_->At(i), typename TypeParam::Type(3));
  }
}

TYPED_TEST(StateDictTest, nested_merge_test)
{
  fetch::ml::StateDict<TypeParam> a;
  fetch::ml::StateDict<TypeParam> b;
  a.dict_["next1"].dict_["nest2"].weights_ =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  b.dict_["next1"].dict_["nest2"].weights_ =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  a.dict_["next1"].dict_["nest2"].weights_->Fill(typename TypeParam::Type(5));
  b.dict_["next1"].dict_["nest2"].weights_->Fill(typename TypeParam::Type(3));
  for (unsigned int i(0); i < 25; ++i)
  {
    EXPECT_EQ(a.dict_["next1"].dict_["nest2"].weights_->At(i), typename TypeParam::Type(5));
    EXPECT_EQ(b.dict_["next1"].dict_["nest2"].weights_->At(i), typename TypeParam::Type(3));
  }
  a.Merge(b);
  for (unsigned int i(0); i < 25; ++i)
  {
    EXPECT_EQ(a.dict_["next1"].dict_["nest2"].weights_->At(i), typename TypeParam::Type(4));
    EXPECT_EQ(b.dict_["next1"].dict_["nest2"].weights_->At(i), typename TypeParam::Type(3));
  }
}
