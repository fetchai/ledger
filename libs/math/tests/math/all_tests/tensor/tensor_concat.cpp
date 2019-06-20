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

#include "gtest/gtest.h"

template <typename T>
class TensorConcatenationTest : public ::testing::Test
{
};

using MyTypes =
    ::testing::Types<std::int32_t, std::int64_t, std::uint32_t, std::uint64_t, float, double>;
TYPED_TEST_CASE(TensorConcatenationTest, MyTypes);

template <typename T>
fetch::math::Tensor<T> PrepareTensor(std::vector<fetch::math::SizeType> const &shape)
{
  fetch::math::Tensor<T> t{shape};
  t.FillArange(static_cast<T>(0), static_cast<T>(t.size()));
  return t;
}

template <typename T>
fetch::math::Tensor<T> PrepareGroundTruth2D(std::vector<fetch::math::SizeType> const & shape,
                                            std::vector<fetch::math::Tensor<T>> const &vt,
                                            typename fetch::math::SizeType const &     axis)
{
  using SizeType = fetch::math::SizeType;

  fetch::math::Tensor<T> gt{shape};

  SizeType cur_axis_pos{0};
  T        counter{0};

  for (SizeType m{0}; m < vt.size(); ++m)
  {
    counter = T{0};
    for (SizeType j{0}; j < vt[m].shape()[1]; ++j)
    {
      for (SizeType k{0}; k < vt[m].shape()[0]; ++k)
      {
        if (axis == 0)
        {
          gt.Set(cur_axis_pos + k, j, counter);
        }
        else
        {
          gt.Set(k, cur_axis_pos + j, counter);
        }
        ++counter;
      }
    }
    cur_axis_pos += vt[m].shape()[axis];
  }
  return gt;
}

template <typename T>
fetch::math::Tensor<T> PrepareGroundTruth3D(std::vector<fetch::math::SizeType> const & shape,
                                            std::vector<fetch::math::Tensor<T>> const &vt,
                                            typename fetch::math::SizeType const &     axis)
{
  using SizeType = fetch::math::SizeType;

  fetch::math::Tensor<T> gt{shape};

  SizeType cur_axis_pos{0};
  T        counter{0};

  for (SizeType m{0}; m < vt.size(); ++m)
  {
    counter = T{0};
    for (SizeType i{0}; i < vt[m].shape()[2]; ++i)
    {
      for (SizeType j{0}; j < vt[m].shape()[1]; ++j)
      {
        for (SizeType k{0}; k < vt[m].shape()[0]; ++k)
        {
          if (axis == 0)
          {
            gt.Set(cur_axis_pos + k, j, i, counter);
          }
          else if (axis == 1)
          {
            gt.Set(k, cur_axis_pos + j, i, counter);
          }
          else
          {
            gt.Set(k, j, cur_axis_pos + i, counter);
          }
          ++counter;
        }
      }
    }
    cur_axis_pos += vt[m].shape()[axis];
  }
  return gt;
}

TYPED_TEST(TensorConcatenationTest, tensor_concat_2d)
{
  using Tensor = fetch::math::Tensor<TypeParam>;

  // axis 0 concat
  Tensor t1 = PrepareTensor<TypeParam>({2, 4});
  Tensor t2 = PrepareTensor<TypeParam>({2, 4});
  Tensor t3 = PrepareTensor<TypeParam>({2, 4});

  std::vector<Tensor> vt{t1, t2, t3};
  Tensor              gt = PrepareGroundTruth2D<TypeParam>({6, 4}, vt, 0);

  Tensor ret = Tensor::Concat(vt, 0);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));

  // axis 1 concat
  t1 = PrepareTensor<TypeParam>({2, 4});
  t2 = PrepareTensor<TypeParam>({2, 4});
  t3 = PrepareTensor<TypeParam>({2, 4});

  vt = {t1, t2, t3};
  gt = PrepareGroundTruth2D<TypeParam>({2, 12}, vt, 1);

  ret = Tensor::Concat(vt, 1);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));
}

TYPED_TEST(TensorConcatenationTest, tensor_concat_3d)
{
  using Tensor = fetch::math::Tensor<TypeParam>;

  // axis 0 concatenation
  Tensor              t1 = PrepareTensor<TypeParam>({3, 2, 2});
  Tensor              t2 = PrepareTensor<TypeParam>({3, 2, 2});
  Tensor              t3 = PrepareTensor<TypeParam>({3, 2, 2});
  std::vector<Tensor> vt{t1, t2, t3};
  Tensor              gt  = PrepareGroundTruth3D({9, 2, 2}, vt, 0);
  Tensor              ret = Tensor::Concat(vt, 0);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));

  // axis 1 concatenation
  t1  = PrepareTensor<TypeParam>({3, 2, 2});
  t2  = PrepareTensor<TypeParam>({3, 2, 2});
  t3  = PrepareTensor<TypeParam>({3, 2, 2});
  vt  = {t1, t2, t3};
  gt  = PrepareGroundTruth3D({3, 6, 2}, vt, 1);
  ret = Tensor::Concat(vt, 1);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));

  // axis 2 concatenation
  t1  = PrepareTensor<TypeParam>({3, 2, 6});
  t2  = PrepareTensor<TypeParam>({3, 2, 6});
  t3  = PrepareTensor<TypeParam>({3, 2, 6});
  vt  = {t1, t2, t3};
  gt  = PrepareGroundTruth3D({3, 2, 18}, vt, 2);
  ret = Tensor::Concat(vt, 2);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));
}

TYPED_TEST(TensorConcatenationTest, tensor_concat_various_sizes)
{
  using Tensor = fetch::math::Tensor<TypeParam>;

  /// 2D Tensors ///
  // axis 0 concatenation
  Tensor              t1 = PrepareTensor<TypeParam>({1, 2});
  Tensor              t2 = PrepareTensor<TypeParam>({3, 2});
  Tensor              t3 = PrepareTensor<TypeParam>({18, 2});
  std::vector<Tensor> vt{t1, t2, t3};
  Tensor              gt  = PrepareGroundTruth2D({22, 2}, vt, 0);
  Tensor              ret = Tensor::Concat(vt, 0);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));

  // axis 1 concatenation
  t1  = PrepareTensor<TypeParam>({2, 2});
  t2  = PrepareTensor<TypeParam>({2, 1});
  t3  = PrepareTensor<TypeParam>({2, 50});
  vt  = {t1, t2, t3};
  gt  = PrepareGroundTruth2D({2, 53}, vt, 1);
  ret = Tensor::Concat(vt, 1);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));

  /// 3D Tensors ///
  // axis 0 concatenation
  t1  = PrepareTensor<TypeParam>({1, 2, 2});
  t2  = PrepareTensor<TypeParam>({9, 2, 2});
  t3  = PrepareTensor<TypeParam>({10, 2, 2});
  vt  = {t1, t2, t3};
  gt  = PrepareGroundTruth3D({20, 2, 2}, vt, 0);
  ret = Tensor::Concat(vt, 0);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));

  // axis 1 concatenation
  t1  = PrepareTensor<TypeParam>({2, 7, 2});
  t2  = PrepareTensor<TypeParam>({2, 2, 2});
  t3  = PrepareTensor<TypeParam>({2, 9, 2});
  vt  = {t1, t2, t3};
  gt  = PrepareGroundTruth3D({2, 18, 2}, vt, 1);
  ret = Tensor::Concat(vt, 1);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));

  // axis 2 concatenation
  t1  = PrepareTensor<TypeParam>({3, 2, 9});
  t2  = PrepareTensor<TypeParam>({3, 2, 2});
  t3  = PrepareTensor<TypeParam>({3, 2, 1});
  vt  = {t1, t2, t3};
  gt  = PrepareGroundTruth3D({3, 2, 12}, vt, 2);
  ret = Tensor::Concat(vt, 2);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));
}

TYPED_TEST(TensorConcatenationTest, tensor_Split_2d)
{
  using Tensor   = fetch::math::Tensor<TypeParam>;
  using SizeType = fetch::math::SizeType;

  SizeType axis = 0;

  // axis 0 concat & Split
  Tensor t1 = PrepareTensor<TypeParam>({2, 4});
  Tensor t2 = PrepareTensor<TypeParam>({2, 4});
  Tensor t3 = PrepareTensor<TypeParam>({2, 4});

  std::vector<Tensor> vt{t1, t2, t3};
  Tensor              gt = PrepareGroundTruth2D<TypeParam>({6, 4}, vt, axis);

  Tensor ret = Tensor::Concat(vt, axis);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));

  std::vector<SizeType> concat_points(3);
  for (SizeType i{0}; i < concat_points.size(); ++i)
  {
    concat_points[i] = vt[i].shape()[axis];
  }
  std::vector<Tensor> ret_tensors = Tensor::Split(ret, concat_points, axis);

  EXPECT_EQ(ret_tensors.size(), vt.size());
  EXPECT_EQ(ret_tensors[0].shape(), t1.shape());
  EXPECT_EQ(ret_tensors[1].shape(), t2.shape());
  EXPECT_EQ(ret_tensors[2].shape(), t3.shape());
  EXPECT_TRUE(ret_tensors[0].AllClose(t1));
  EXPECT_TRUE(ret_tensors[1].AllClose(t2));
  EXPECT_TRUE(ret_tensors[2].AllClose(t3));

  // axis 1 concat & Split
  axis = 1;

  t1 = PrepareTensor<TypeParam>({2, 4});
  t2 = PrepareTensor<TypeParam>({2, 4});
  t3 = PrepareTensor<TypeParam>({2, 4});

  vt = {t1, t2, t3};
  gt = PrepareGroundTruth2D<TypeParam>({2, 12}, vt, axis);

  ret = Tensor::Concat(vt, axis);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));

  for (SizeType i{0}; i < concat_points.size(); ++i)
  {
    concat_points[i] = vt[i].shape()[axis];
  }
  ret_tensors = Tensor::Split(ret, concat_points, axis);

  EXPECT_EQ(ret_tensors.size(), vt.size());
  EXPECT_EQ(ret_tensors[0].shape(), t1.shape());
  EXPECT_EQ(ret_tensors[1].shape(), t2.shape());
  EXPECT_EQ(ret_tensors[2].shape(), t3.shape());
  EXPECT_TRUE(ret_tensors[0].AllClose(t1));
  EXPECT_TRUE(ret_tensors[1].AllClose(t2));
  EXPECT_TRUE(ret_tensors[2].AllClose(t3));
}

TYPED_TEST(TensorConcatenationTest, tensor_Split_3d)
{
  using Tensor   = fetch::math::Tensor<TypeParam>;
  using SizeType = fetch::math::SizeType;

  SizeType axis = 0;

  // axis 0 concat & Split
  Tensor t1 = PrepareTensor<TypeParam>({2, 4, 2});
  Tensor t2 = PrepareTensor<TypeParam>({2, 4, 2});
  Tensor t3 = PrepareTensor<TypeParam>({2, 4, 2});

  std::vector<Tensor> vt{t1, t2, t3};
  Tensor              gt = PrepareGroundTruth3D<TypeParam>({6, 4, 2}, vt, axis);

  Tensor ret = Tensor::Concat(vt, axis);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));

  std::vector<SizeType> concat_points(3);
  for (SizeType i{0}; i < concat_points.size(); ++i)
  {
    concat_points[i] = vt[i].shape()[axis];
  }
  std::vector<Tensor> ret_tensors = Tensor::Split(ret, concat_points, axis);

  EXPECT_EQ(ret_tensors.size(), vt.size());
  EXPECT_EQ(ret_tensors[0].shape(), t1.shape());
  EXPECT_EQ(ret_tensors[1].shape(), t2.shape());
  EXPECT_EQ(ret_tensors[2].shape(), t3.shape());
  EXPECT_TRUE(ret_tensors[0].AllClose(t1));
  EXPECT_TRUE(ret_tensors[1].AllClose(t2));
  EXPECT_TRUE(ret_tensors[2].AllClose(t3));

  // axis 1 concat & Split
  axis = 1;

  t1 = PrepareTensor<TypeParam>({2, 4, 2});
  t2 = PrepareTensor<TypeParam>({2, 4, 2});
  t3 = PrepareTensor<TypeParam>({2, 4, 2});

  vt = {t1, t2, t3};
  gt = PrepareGroundTruth3D<TypeParam>({2, 12, 2}, vt, axis);

  ret = Tensor::Concat(vt, axis);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));

  for (SizeType i{0}; i < concat_points.size(); ++i)
  {
    concat_points[i] = vt[i].shape()[axis];
  }
  ret_tensors = Tensor::Split(ret, concat_points, axis);

  EXPECT_EQ(ret_tensors.size(), vt.size());
  EXPECT_EQ(ret_tensors[0].shape(), t1.shape());
  EXPECT_EQ(ret_tensors[1].shape(), t2.shape());
  EXPECT_EQ(ret_tensors[2].shape(), t3.shape());
  EXPECT_TRUE(ret_tensors[0].AllClose(t1));
  EXPECT_TRUE(ret_tensors[1].AllClose(t2));
  EXPECT_TRUE(ret_tensors[2].AllClose(t3));

  // axis 2 concat & Split
  axis = 2;

  t1 = PrepareTensor<TypeParam>({2, 4, 2});
  t2 = PrepareTensor<TypeParam>({2, 4, 2});
  t3 = PrepareTensor<TypeParam>({2, 4, 2});

  vt = {t1, t2, t3};
  gt = PrepareGroundTruth3D<TypeParam>({2, 4, 6}, vt, axis);

  ret = Tensor::Concat(vt, axis);

  EXPECT_TRUE(ret.shape() == gt.shape());
  EXPECT_TRUE(ret.AllClose(gt));

  for (SizeType i{0}; i < concat_points.size(); ++i)
  {
    concat_points[i] = vt[i].shape()[axis];
  }
  ret_tensors = Tensor::Split(ret, concat_points, axis);

  EXPECT_EQ(ret_tensors.size(), vt.size());
  EXPECT_EQ(ret_tensors[0].shape(), t1.shape());
  EXPECT_EQ(ret_tensors[1].shape(), t2.shape());
  EXPECT_EQ(ret_tensors[2].shape(), t3.shape());
  EXPECT_TRUE(ret_tensors[0].AllClose(t1));
  EXPECT_TRUE(ret_tensors[1].AllClose(t2));
  EXPECT_TRUE(ret_tensors[2].AllClose(t3));
}
