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

#include "core/serializers/main_serializer_definition.hpp"
#include "gtest/gtest.h"
#include "ml/ops/loss_functions/softmax_cross_entropy_loss.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include <memory>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class SoftmaxCrossEntropyTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SoftmaxCrossEntropyTest, math::test::HighPrecisionTensorFloatingTypes);

TYPED_TEST(SoftmaxCrossEntropyTest, perfect_match_forward_test)
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType n_classes     = 3;
  SizeType n_data_points = 1;

  TypeParam data1({n_classes, n_data_points});
  TypeParam data2({n_classes, n_data_points});

  // these are not logits - a softmax will get called on this
  data1.At(0, 0) = DataType{0};
  data1.At(1, 0) = DataType{0};
  data1.At(2, 0) = DataType{999999};

  //
  data2.At(0, 0) = DataType{0};
  data2.At(1, 0) = DataType{0};
  data2.At(2, 0) = DataType{1};

  fetch::ml::ops::SoftmaxCrossEntropyLoss<TypeParam> op;
  TypeParam                                          result({1, 1});
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, result);

  EXPECT_EQ(result(0, 0), DataType{0});
}

TYPED_TEST(SoftmaxCrossEntropyTest, simple_forward_test)
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType n_classes     = 4;
  SizeType n_data_points = 4;

  TypeParam data1(std::vector<SizeType>{n_classes, n_data_points});

  TypeParam data2(std::vector<SizeType>{n_classes, n_data_points});
  data2.Fill(DataType{0});
  data2.At(1, 0) = DataType{1};
  data2.At(2, 1) = DataType{1};
  data2.At(3, 2) = DataType{1};
  data2.At(0, 3) = DataType{1};

  std::vector<double> vals{0.1,  0.8,  0.05, 0.05, 0.2, 0.5, 0.2, 0.1,
                           0.05, 0.05, 0.8,  0.1,  0.5, 0.1, 0.1, 0.3};
  SizeType            idx_count = 0;
  for (SizeType i = 0; i < n_data_points; ++i)
  {
    for (SizeType j = 0; j < n_classes; ++j)
    {
      data1.Set(j, i, fetch::math::AsType<DataType>(vals[idx_count]));
      ++idx_count;
    }
  }

  fetch::ml::ops::SoftmaxCrossEntropyLoss<TypeParam> op;
  TypeParam                                          result({1, 1});
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, result);

  ASSERT_FLOAT_EQ(static_cast<float>(result(0, 0)),
                  static_cast<float>((1.4480233671411693 + 0.8925382250479597 + 1.5925382250479596 +
                                      1.1503729081395468) /
                                     static_cast<double>(n_data_points)));
}

TYPED_TEST(SoftmaxCrossEntropyTest, trivial_one_dimensional_backward_test)
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType n_classes     = 3;
  SizeType n_data_points = 1;

  TypeParam data1(std::vector<SizeType>{n_classes, n_data_points});
  TypeParam data2(std::vector<SizeType>{n_classes, n_data_points});
  TypeParam gt(std::vector<SizeType>{n_classes, n_data_points});

  // set gt data
  std::vector<double> gt_data{0.10650698, -0.89349302, 0.78698604};

  for (SizeType i = 0; i < gt.size(); ++i)
  {
    gt.Set(i, SizeType{0}, fetch::math::AsType<DataType>(gt_data[i]));
  }

  std::vector<double> unscaled_vals{-1.0, -1.0, 1.0};
  std::vector<double> targets{0.0, 1.0, 0.0};

  for (SizeType i = 0; i < n_data_points * n_classes; ++i)
  {
    data1.Set(i, SizeType{0}, fetch::math::AsType<DataType>(unscaled_vals[i]));
    data2.Set(i, SizeType{0}, fetch::math::AsType<DataType>(targets[i]));
  }

  TypeParam error_signal({1, 1});
  error_signal(0, 0) = DataType{1};

  fetch::ml::ops::SoftmaxCrossEntropyLoss<TypeParam> op;

  EXPECT_TRUE(op.Backward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)},
                          error_signal)
                  .at(0)
                  .AllClose(gt, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SoftmaxCrossEntropyTest, backward_test)
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType n_classes     = 4;
  SizeType n_data_points = 4;

  TypeParam data1(std::vector<SizeType>{n_classes, n_data_points});
  TypeParam data2(std::vector<SizeType>{n_classes, n_data_points});
  TypeParam gt(std::vector<SizeType>{n_classes, n_data_points});

  /// python script computing these values can be found at
  /// scripts/python_ml_lib/cross_entropy_test.py
  gt.Fill(DataType{0});
  std::vector<double> gt_vals{
      0.20340865850448608398, 0.30961471796035766602, 0.19348828494548797607,
      0.19348828494548797607, 0.23503439128398895264, 0.31726324558258056641,
      0.13503438234329223633, 0.21266791224479675293, 0.19348828494548797607,
      0.19348828494548797607, 0.40961471199989318848, 0.1034086570143699646,
      0.2165187150239944458,  0.21216882765293121338, 0.21216882765293121338,
      0.25914362072944641113};

  SizeType idx_count = 0;
  for (SizeType i = 0; i < n_classes; ++i)
  {
    for (SizeType j = 0; j < n_data_points; ++j)
    {
      gt.Set(j, i, fetch::math::AsType<DataType>(gt_vals[idx_count]));
      ++idx_count;
    }
  }

  std::vector<double> vals{0.1,  0.8,  0.05, 0.05, 0.2, 0.5, 0.2, 0.1,
                           0.05, 0.05, 0.8,  0.1,  0.5, 0.1, 0.1, 0.3};
  std::vector<double> err{0.0, 0.1, 0.0, 0.0, 0.0, 0.0, 0.1, 0.0,
                          0.0, 0.0, 0.0, 0.1, 0.1, 0.0, 0.0, 0.0};
  idx_count = 0;
  for (SizeType i = 0; i < n_classes; ++i)
  {
    for (SizeType j = 0; j < n_data_points; ++j)
    {
      data1.Set(j, i, fetch::math::AsType<DataType>(vals[idx_count]));
      data2.Set(j, i, fetch::math::AsType<DataType>(err[idx_count]));
      ++idx_count;
    }
  }

  fetch::ml::ops::SoftmaxCrossEntropyLoss<TypeParam> op;

  TypeParam error_signal({1, 1});
  error_signal(0, 0) = DataType{1};

  EXPECT_TRUE(op.Backward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)},
                          error_signal)
                  .at(0)
                  .AllClose(gt, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SoftmaxCrossEntropyTest, saveparams_test)
{
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::SoftmaxCrossEntropyLoss<TensorType>::SPType;
  using OpType     = typename fetch::ml::ops::SoftmaxCrossEntropyLoss<TensorType>;

  SizeType n_classes     = 4;
  SizeType n_data_points = 4;

  TypeParam data1(std::vector<SizeType>{n_data_points, n_classes});

  TypeParam data2(std::vector<SizeType>{n_data_points, n_classes});
  data2.Fill(DataType{0});
  data2.At(0, 1) = DataType{1};
  data2.At(1, 2) = DataType{1};
  data2.At(2, 3) = DataType{1};
  data2.At(3, 0) = DataType{1};

  std::vector<double> vals{0.1,  0.8,  0.05, 0.05, 0.2, 0.5, 0.2, 0.1,
                           0.05, 0.05, 0.8,  0.1,  0.5, 0.1, 0.1, 0.3};
  SizeType            idx_count = 0;
  for (SizeType i = 0; i < n_data_points; ++i)
  {
    for (SizeType j = 0; j < n_classes; ++j)
    {
      data1.Set(i, j, fetch::math::AsType<DataType>(vals[idx_count]));
      ++idx_count;
    }
  }

  OpType    op;
  TypeParam result({1, 1});
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, result);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original graph
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, result);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TypeParam new_result({1, 1});
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, new_result);

  // test correct values
  EXPECT_NEAR(static_cast<double>(result(0, 0)), static_cast<double>(new_result(0, 0)),
              static_cast<double>(0));
}

TYPED_TEST(SoftmaxCrossEntropyTest, saveparams_backward_test)
{
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::SoftmaxCrossEntropyLoss<TensorType>::SPType;
  using OpType     = typename fetch::ml::ops::SoftmaxCrossEntropyLoss<TensorType>;

  SizeType n_classes     = 4;
  SizeType n_data_points = 4;

  TypeParam data1(std::vector<SizeType>{n_classes, n_data_points});
  TypeParam data2(std::vector<SizeType>{n_classes, n_data_points});
  TypeParam gt(std::vector<SizeType>{n_classes, n_data_points});

  /// python script computing these values can be found at
  /// scripts/python_ml_lib/cross_entropy_test.py
  gt.Fill(DataType{0});
  std::vector<double> gt_vals{
      0.20340865850448608398, 0.30961471796035766602, 0.19348828494548797607,
      0.19348828494548797607, 0.23503439128398895264, 0.31726324558258056641,
      0.13503438234329223633, 0.21266791224479675293, 0.19348828494548797607,
      0.19348828494548797607, 0.40961471199989318848, 0.1034086570143699646,
      0.2165187150239944458,  0.21216882765293121338, 0.21216882765293121338,
      0.25914362072944641113};

  SizeType idx_count = 0;
  for (SizeType i = 0; i < n_classes; ++i)
  {
    for (SizeType j = 0; j < n_data_points; ++j)
    {
      gt.Set(j, i, fetch::math::AsType<DataType>(gt_vals[idx_count]));
      ++idx_count;
    }
  }

  std::vector<double> vals{0.1,  0.8,  0.05, 0.05, 0.2, 0.5, 0.2, 0.1,
                           0.05, 0.05, 0.8,  0.1,  0.5, 0.1, 0.1, 0.3};
  std::vector<double> err{0.0, 0.1, 0.0, 0.0, 0.0, 0.0, 0.1, 0.0,
                          0.0, 0.0, 0.0, 0.1, 0.1, 0.0, 0.0, 0.0};
  idx_count = 0;
  for (SizeType i = 0; i < n_classes; ++i)
  {
    for (SizeType j = 0; j < n_data_points; ++j)
    {
      data1.Set(j, i, fetch::math::AsType<DataType>(vals[idx_count]));
      data2.Set(j, i, fetch::math::AsType<DataType>(err[idx_count]));
      ++idx_count;
    }
  }

  fetch::ml::ops::SoftmaxCrossEntropyLoss<TypeParam> op;

  TypeParam error_signal({1, 1});
  error_signal(0, 0) = DataType{1};

  // run op once to make sure caches etc. have been filled. Otherwise the test might be trivial!
  std::vector<TypeParam> gradients = op.Backward(
      {std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, error_signal);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  gradients = op.Backward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)},
                          error_signal);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TypeParam> new_gradients = new_op.Backward(
      {std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, error_signal);

  // test correct values
  EXPECT_TRUE(gradients.at(0).AllClose(new_gradients.at(0),
                                       fetch::math::function_tolerance<DataType>() * DataType{4},
                                       fetch::math::function_tolerance<DataType>() * DataType{4}));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
