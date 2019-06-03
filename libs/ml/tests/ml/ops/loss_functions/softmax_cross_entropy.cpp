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
#include "ml/ops/loss_functions/softmax_cross_entropy.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include <gtest/gtest.h>

template <typename T>
class SoftmaxCrossEntropyTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(SoftmaxCrossEntropyTest, MyTypes);

TYPED_TEST(SoftmaxCrossEntropyTest, perfect_match_forward_test)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType n_classes     = 3;
  SizeType n_data_points = 1;

  TypeParam data1({n_data_points, n_classes});
  TypeParam gt({n_data_points, n_classes});

  // these are not logits - a softmax will get called on this
  data1.At(0, 0) = DataType(0);
  data1.At(0, 1) = DataType(0);
  data1.At(0, 2) = DataType(999999.);

  //
  gt.At(0, 0) = DataType(0);
  gt.At(0, 1) = DataType(0);
  gt.At(0, 2) = DataType(1);

  fetch::ml::ops::SoftmaxCrossEntropy<TypeParam> op;
  EXPECT_EQ(op.Forward({data1, gt}), DataType(0));
}

TYPED_TEST(SoftmaxCrossEntropyTest, simple_forward_test)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType n_classes     = 4;
  SizeType n_data_points = 4;

  TypeParam data1(std::vector<SizeType>{n_data_points, n_classes});

  TypeParam gt(std::vector<SizeType>{n_data_points, n_classes});
  gt.Fill(DataType(0));
  gt.At(0, 1) = DataType(1);
  gt.At(1, 2) = DataType(1);
  gt.At(2, 3) = DataType(1);
  gt.At(3, 0) = DataType(1);

  std::vector<double> vals{0.1,  0.8,  0.05, 0.05, 0.2, 0.5, 0.2, 0.1,
                           0.05, 0.05, 0.8,  0.1,  0.5, 0.1, 0.1, 0.3};
  SizeType            idx_count = 0;
  for (SizeType i = 0; i < n_data_points; ++i)
  {
    for (SizeType j = 0; j < n_classes; ++j)
    {
      data1.Set(i, j, DataType(vals[idx_count]));
      ++idx_count;
    }
  }

  fetch::ml::ops::SoftmaxCrossEntropy<TypeParam> op;
  ASSERT_FLOAT_EQ(float(op.Forward({data1, gt})), float((1.4480233671411693 + 0.8925382250479597 +
                                                         1.5925382250479596 + 1.1503729081395468) /
                                                        double(n_data_points)));
}

TYPED_TEST(SoftmaxCrossEntropyTest, trivial_one_dimensional_backward_test)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType n_classes     = 3;
  SizeType n_data_points = 1;

  TypeParam data1(std::vector<SizeType>{n_data_points, n_classes});
  TypeParam data2(std::vector<SizeType>{n_data_points, n_classes});
  TypeParam gt(std::vector<SizeType>{n_data_points, n_classes});

  // set gt data
  std::vector<double> gt_data{0.10650698, -0.89349302, 0.78698604};

  for (SizeType i = 0; i < gt.size(); ++i)
  {
    gt.Set(SizeType{0}, i, DataType(gt_data[i]));
  }

  std::vector<double> unscaled_vals{-1.0, -1.0, 1.0};
  std::vector<double> targets{0.0, 1.0, 0.0};

  for (SizeType i = 0; i < n_data_points * n_classes; ++i)
  {
    data1.Set(SizeType{0}, i, DataType(unscaled_vals[i]));
    data2.Set(SizeType{0}, i, DataType(targets[i]));
  }

  fetch::ml::ops::SoftmaxCrossEntropy<TypeParam> op;
  EXPECT_TRUE(op.Backward({data1, data2}).AllClose(gt, DataType(1e-5), DataType(1e-5)));
}

TYPED_TEST(SoftmaxCrossEntropyTest, backward_test)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType n_classes     = 4;
  SizeType n_data_points = 4;

  TypeParam data1(std::vector<SizeType>{n_data_points, n_classes});
  TypeParam err_sig(std::vector<SizeType>{n_data_points, n_classes});
  TypeParam gt(std::vector<SizeType>{n_data_points, n_classes});

  /// python script computing these values can be found at
  /// scripts/python_ml_lib/cross_entropy_test.py
  gt.Fill(DataType(0));
  std::vector<double> gt_vals{
      0.20340865850448608398, 0.30961471796035766602, 0.19348828494548797607,
      0.19348828494548797607, 0.23503439128398895264, 0.31726324558258056641,
      0.13503438234329223633, 0.21266791224479675293, 0.19348828494548797607,
      0.19348828494548797607, 0.40961471199989318848, 0.1034086570143699646,
      0.2165187150239944458,  0.21216882765293121338, 0.21216882765293121338,
      0.25914362072944641113};

  SizeType idx_count = 0;
  for (SizeType i = 0; i < n_data_points; ++i)
  {
    for (SizeType j = 0; j < n_classes; ++j)
    {
      gt.Set(i, j, DataType(gt_vals[idx_count]));
      ++idx_count;
    }
  }

  std::vector<double> vals{0.1,  0.8,  0.05, 0.05, 0.2, 0.5, 0.2, 0.1,
                           0.05, 0.05, 0.8,  0.1,  0.5, 0.1, 0.1, 0.3};
  std::vector<double> err{0.0, 0.1, 0.0, 0.0, 0.0, 0.0, 0.1, 0.0,
                          0.0, 0.0, 0.0, 0.1, 0.1, 0.0, 0.0, 0.0};
  idx_count = 0;
  for (SizeType i = 0; i < n_data_points; ++i)
  {
    for (SizeType j = 0; j < n_classes; ++j)
    {
      data1.Set(i, j, DataType(vals[idx_count]));
      err_sig.Set(i, j, DataType(err[idx_count]));
      ++idx_count;
    }
  }

  fetch::ml::ops::SoftmaxCrossEntropy<TypeParam> op;

  EXPECT_TRUE(op.Backward({data1, err_sig}).AllClose(gt, DataType(1e-7), DataType(1e-7)));
}
