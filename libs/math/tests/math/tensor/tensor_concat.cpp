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
#include <gtest/gtest.h>

using namespace std;
using namespace std::chrono;

template <typename T>
class TensorConcatenationTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<std::int32_t, std::int64_t, std::uint32_t, std::uint64_t, float, double>;
TYPED_TEST_CASE(TensorConcatenationTest, MyTypes);

TYPED_TEST(TensorConcatenationTest, tensor_concat_2d_axis_0)
{
  using Tensor = fetch::math::Tensor<TypeParam>;
  Tensor t1 = Tensor::FromString("0 1 2 3; 4 5 6 7");
  Tensor t2 = Tensor::FromString("0 1 2 3; 4 5 6 7");
  Tensor t3 = Tensor::FromString("0 1 2 3; 4 5 6 7");
  Tensor gt = Tensor::FromString("0 1 2 3; 4 5 6 7; 0 1 2 3; 4 5 6 7; 0 1 2 3; 4 5 6 7");

  std::vector<Tensor> vt{t1, t2, t3};
  Tensor ret = Tensor::Concat(vt,
          0);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));
}

TYPED_TEST(TensorConcatenationTest, tensor_concat_2d_axis_1)
{
  using Tensor = fetch::math::Tensor<TypeParam>;
  Tensor t1 = Tensor::FromString("0 1 2 3; 4 5 6 0");
  Tensor t2 = Tensor::FromString("0 1 2 3; 4 5 6 0");
  Tensor t3 = Tensor::FromString("0 1 2 3; 4 5 6 0");
  Tensor gt = Tensor::FromString("0 1 2 3 0 1 2 3 0 1 2 3; 4 5 6 0 4 5 6 0 4 5 6 0");

  std::vector<Tensor> vt{t1, t2, t3};
  Tensor ret = Tensor::Concat(vt, 1);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));
}


TYPED_TEST(TensorConcatenationTest, tensor_concat_3d_axis_0)
{
  using Tensor = fetch::math::Tensor<TypeParam>;
  using SizeType = fetch::math::SizeType;

  Tensor t1{{3, 2, 2}};
  Tensor t2{{3, 2, 2}};
  Tensor t3{{3, 2, 2}};
  Tensor gt{{9, 2, 2}};

  TypeParam counter{0};
  auto t1_it = t1.begin();
  auto t2_it = t2.begin();
  auto t3_it = t3.begin();
  while (t1_it.is_valid())
  {
    *t1_it = counter;
    *t2_it = counter;
    *t3_it = counter;
    ++counter;
    ++t1_it;
    ++t2_it;
    ++t3_it;
  }

  std::vector<Tensor> vt{t1, t2, t3};
  SizeType cur_axis_pos{0};
  for (SizeType m{0}; m < 3; ++m)
  {
    counter = TypeParam{0};
    for (SizeType i{0}; i < t1.shape()[2]; ++i)
    {
      for (SizeType j{0}; j < t1.shape()[1]; ++j)
      {
        for (SizeType k{0}; k < t1.shape()[0]; ++k)
        {
          gt.Set(cur_axis_pos + k, j, i, counter);
          ++counter;
        }
      }
    }
    cur_axis_pos += vt[m].shape()[0];
  }
  Tensor ret = Tensor::Concat(vt, 0);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));
}


TYPED_TEST(TensorConcatenationTest, tensor_concat_3d_axis_1)
{
  using Tensor = fetch::math::Tensor<TypeParam>;
  using SizeType = fetch::math::SizeType;

  Tensor t1{{3, 2, 2}};
  Tensor t2{{3, 2, 2}};
  Tensor t3{{3, 2, 2}};
  Tensor gt{{3, 6, 2}};

  TypeParam counter{0};
  auto t1_it = t1.begin();
  auto t2_it = t2.begin();
  auto t3_it = t3.begin();
  while (t1_it.is_valid())
  {
    *t1_it = counter;
    *t2_it = counter;
    *t3_it = counter;
    ++counter;
    ++t1_it;
    ++t2_it;
    ++t3_it;
  }

  std::vector<Tensor> vt{t1, t2, t3};
  SizeType cur_axis_pos{0};
  for (SizeType m{0}; m < 3; ++m)
  {
    counter = TypeParam{0};
    for (SizeType i{0}; i < vt[m].shape()[2]; ++i)
    {
      for (SizeType j{0}; j < vt[m].shape()[1]; ++j)
      {
        for (SizeType k{0}; k < vt[m].shape()[0]; ++k)
        {
          gt.Set(k, cur_axis_pos + j, i, counter);
          ++counter;
        }
      }
    }
    cur_axis_pos += vt[m].shape()[1];
  }
  Tensor ret = Tensor::Concat(vt, 1);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));
}


TYPED_TEST(TensorConcatenationTest, tensor_concat_3d_axis_2)
{
  using Tensor = fetch::math::Tensor<TypeParam>;
  using SizeType = fetch::math::SizeType;

  Tensor t1{{3, 2, 6}};
  Tensor t2{{3, 2, 6}};
  Tensor t3{{3, 2, 6}};
  Tensor gt{{3, 2, 18}};

  TypeParam counter{0};
  auto t1_it = t1.begin();
  auto t2_it = t2.begin();
  auto t3_it = t3.begin();
  while (t1_it.is_valid())
  {
    *t1_it = counter;
    *t2_it = counter;
    *t3_it = counter;
    ++counter;
    ++t1_it;
    ++t2_it;
    ++t3_it;
  }

  std::vector<Tensor> vt{t1, t2, t3};
  SizeType cur_axis_pos{0};
  for (SizeType m{0}; m < 3; ++m)
  {
    counter = TypeParam{0};
    for (SizeType i{0}; i < vt[m].shape()[2]; ++i)
    {
      for (SizeType j{0}; j < vt[m].shape()[1]; ++j)
      {
        for (SizeType k{0}; k < vt[m].shape()[0]; ++k)
        {
          gt.Set(k, j, cur_axis_pos + i, counter);
          ++counter;
        }
      }
    }
    cur_axis_pos += vt[m].shape()[2];
  }

  Tensor ret = Tensor::Concat(vt, 2);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));
}


TYPED_TEST(TensorConcatenationTest, tensor_concat_3d_axis_1_different_sizes)
{
  using Tensor = fetch::math::Tensor<TypeParam>;
  using SizeType = fetch::math::SizeType;

  Tensor t1{{3, 2, 2}};
  Tensor t2{{3, 5, 2}};
  Tensor t3{{3, 1, 2}};
  Tensor gt{{3, 8, 2}};

  TypeParam counter{0};
  auto t1_it = t1.begin();
  auto t2_it = t2.begin();
  auto t3_it = t3.begin();
  while (t1_it.is_valid())
  {
    *t1_it = counter;
    *t2_it = counter;
    *t3_it = counter;
    ++counter;
    ++t1_it;
    ++t2_it;
    ++t3_it;
  }

  std::vector<Tensor> vt{t1, t2, t3};
  SizeType cur_axis_pos{0};
  for (SizeType m{0}; m < 3; ++m)
  {
    counter = TypeParam{0};
    for (SizeType i{0}; i < vt[m].shape()[2]; ++i)
    {
      for (SizeType j{0}; j < vt[m].shape()[1]; ++j)
      {
        for (SizeType k{0}; k < vt[m].shape()[0]; ++k)
        {
          gt.Set(k, cur_axis_pos + j, i, counter);
          ++counter;
        }
      }
    }
    cur_axis_pos += vt[m].shape()[1];
  }
  Tensor ret = Tensor::Concat(vt, 1);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));
}
