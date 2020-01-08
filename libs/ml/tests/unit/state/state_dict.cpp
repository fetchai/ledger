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
#include "ml/state_dict.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class StateDictTest : public ::testing::Test
{
};

TYPED_TEST_CASE(StateDictTest, math::test::TensorFloatingTypes);

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
  for (uint32_t i(0); i < 5; ++i)
  {
    for (uint32_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(a.weights_->At(i, j), typename TypeParam::Type(5));
      EXPECT_EQ(b.weights_->At(i, j), typename TypeParam::Type(3));
    }
  }
  a.Merge(b);
  for (uint32_t i(0); i < 5; ++i)
  {
    for (uint32_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(a.weights_->At(i, j), typename TypeParam::Type(4));
      EXPECT_EQ(b.weights_->At(i, j), typename TypeParam::Type(3));
    }
  }
}

TYPED_TEST(StateDictTest, nested_merge_test)
{
  fetch::ml::StateDict<TypeParam> a;
  fetch::ml::StateDict<TypeParam> b;
  a.dict_["nest1"].dict_["nest2"].weights_ =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  b.dict_["nest1"].dict_["nest2"].weights_ =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  a.dict_["nest1"].dict_["nest2"].weights_->Fill(typename TypeParam::Type(5));
  b.dict_["nest1"].dict_["nest2"].weights_->Fill(typename TypeParam::Type(3));
  for (uint32_t i(0); i < 5; ++i)
  {
    for (uint32_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(a.dict_["nest1"].dict_["nest2"].weights_->At(i, j), typename TypeParam::Type(5));
      EXPECT_EQ(b.dict_["nest1"].dict_["nest2"].weights_->At(i, j), typename TypeParam::Type(3));
    }
  }
  a.Merge(b);
  for (uint32_t i(0); i < 5; ++i)
  {
    for (uint32_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(a.dict_["nest1"].dict_["nest2"].weights_->At(i, j), typename TypeParam::Type(4));
      EXPECT_EQ(b.dict_["nest1"].dict_["nest2"].weights_->At(i, j), typename TypeParam::Type(3));
    }
  }
}

TYPED_TEST(StateDictTest, inline_add_test)
{
  fetch::ml::StateDict<TypeParam> a;
  fetch::ml::StateDict<TypeParam> b;
  a.weights_ = std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  b.weights_ = std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  a.weights_->Fill(typename TypeParam::Type(5));
  b.weights_->Fill(typename TypeParam::Type(3));
  for (uint32_t i(0); i < 5; ++i)
  {
    for (uint32_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(a.weights_->At(i, j), typename TypeParam::Type(5));
      EXPECT_EQ(b.weights_->At(i, j), typename TypeParam::Type(3));
    }
  }
  a.InlineAdd(b);
  for (uint32_t i(0); i < 5; ++i)
  {
    for (uint32_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(a.weights_->At(i, j), typename TypeParam::Type(8));
      EXPECT_EQ(b.weights_->At(i, j), typename TypeParam::Type(3));
    }
  }
}

TYPED_TEST(StateDictTest, nested_inline_add_test)
{
  fetch::ml::StateDict<TypeParam> a;
  fetch::ml::StateDict<TypeParam> b;
  a.dict_["nest1"].dict_["nest2"].weights_ =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  b.dict_["nest1"].dict_["nest2"].weights_ =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  a.dict_["nest1"].dict_["nest2"].weights_->Fill(typename TypeParam::Type(5));
  b.dict_["nest1"].dict_["nest2"].weights_->Fill(typename TypeParam::Type(3));
  for (uint32_t i(0); i < 5; ++i)
  {
    for (uint32_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(a.dict_["nest1"].dict_["nest2"].weights_->At(i, j), typename TypeParam::Type(5));
      EXPECT_EQ(b.dict_["nest1"].dict_["nest2"].weights_->At(i, j), typename TypeParam::Type(3));
    }
  }
  a.InlineAdd(b);
  for (uint32_t i(0); i < 5; ++i)
  {
    for (uint32_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(a.dict_["nest1"].dict_["nest2"].weights_->At(i, j), typename TypeParam::Type(8));
      EXPECT_EQ(b.dict_["nest1"].dict_["nest2"].weights_->At(i, j), typename TypeParam::Type(3));
    }
  }
}

TYPED_TEST(StateDictTest, inline_add_non_strict_test)
{
  fetch::ml::StateDict<TypeParam> a;
  fetch::ml::StateDict<TypeParam> b;
  b.weights_ = std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  b.weights_->Fill(typename TypeParam::Type(3));
  a.InlineAdd(b, false);
  for (uint32_t i(0); i < 5; ++i)
  {
    for (uint32_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(a.weights_->At(i, j), typename TypeParam::Type(3));
      EXPECT_EQ(b.weights_->At(i, j), typename TypeParam::Type(3));
    }
  }
}

TYPED_TEST(StateDictTest, merge_vector_test)
{
  fetch::ml::StateDict<TypeParam> a;
  fetch::ml::StateDict<TypeParam> b;
  fetch::ml::StateDict<TypeParam> c;
  fetch::ml::StateDict<TypeParam> d;

  a.weights_ = std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  b.weights_ = std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  c.weights_ = std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  d.weights_ = std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));

  a.weights_->Fill(typename TypeParam::Type(2));
  b.weights_->Fill(typename TypeParam::Type(4));
  c.weights_->Fill(typename TypeParam::Type(6));
  d.weights_->Fill(typename TypeParam::Type(8));

  std::vector<fetch::ml::StateDict<TypeParam>> state_dicts;
  state_dicts.push_back(a);
  state_dicts.push_back(b);
  state_dicts.push_back(c);
  state_dicts.push_back(d);
  fetch::ml::StateDict<TypeParam> res = fetch::ml::StateDict<TypeParam>::Merge(state_dicts);

  for (uint32_t i(0); i < 5; ++i)
  {
    for (uint32_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(a.weights_->At(i, j), typename TypeParam::Type(2));
      EXPECT_EQ(b.weights_->At(i, j), typename TypeParam::Type(4));
      EXPECT_EQ(c.weights_->At(i, j), typename TypeParam::Type(6));
      EXPECT_EQ(d.weights_->At(i, j), typename TypeParam::Type(8));
      EXPECT_EQ(res.weights_->At(i, j), typename TypeParam::Type(5));
    }
  }
}

TYPED_TEST(StateDictTest, nested_merge_vector_test)
{
  fetch::ml::StateDict<TypeParam> a;
  fetch::ml::StateDict<TypeParam> b;
  fetch::ml::StateDict<TypeParam> c;
  fetch::ml::StateDict<TypeParam> d;

  a.dict_["nest1"].dict_["nest2"].weights_ =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  b.dict_["nest1"].dict_["nest2"].weights_ =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  c.dict_["nest1"].dict_["nest2"].weights_ =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));
  d.dict_["nest1"].dict_["nest2"].weights_ =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 5}));

  a.dict_["nest1"].dict_["nest2"].weights_->Fill(typename TypeParam::Type(2));
  b.dict_["nest1"].dict_["nest2"].weights_->Fill(typename TypeParam::Type(4));
  c.dict_["nest1"].dict_["nest2"].weights_->Fill(typename TypeParam::Type(6));
  d.dict_["nest1"].dict_["nest2"].weights_->Fill(typename TypeParam::Type(8));

  std::vector<fetch::ml::StateDict<TypeParam>> state_dicts;
  state_dicts.push_back(a);
  state_dicts.push_back(b);
  state_dicts.push_back(c);
  state_dicts.push_back(d);
  fetch::ml::StateDict<TypeParam> res = fetch::ml::StateDict<TypeParam>::Merge(state_dicts);

  for (uint32_t i(0); i < 5; ++i)
  {
    for (uint32_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(a.dict_["nest1"].dict_["nest2"].weights_->At(i, j), typename TypeParam::Type(2));
      EXPECT_EQ(b.dict_["nest1"].dict_["nest2"].weights_->At(i, j), typename TypeParam::Type(4));
      EXPECT_EQ(c.dict_["nest1"].dict_["nest2"].weights_->At(i, j), typename TypeParam::Type(6));
      EXPECT_EQ(d.dict_["nest1"].dict_["nest2"].weights_->At(i, j), typename TypeParam::Type(8));
      EXPECT_EQ(res.dict_["nest1"].dict_["nest2"].weights_->At(i, j), typename TypeParam::Type(5));
    }
  }
}
}  // namespace test
}  // namespace ml
}  // namespace fetch
