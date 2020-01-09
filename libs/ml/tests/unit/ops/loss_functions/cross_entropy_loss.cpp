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
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include <memory>

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class CrossEntropyTest : public ::testing::Test
{
};

TYPED_TEST_CASE(CrossEntropyTest, math::test::HighPrecisionTensorFloatingTypes);

TYPED_TEST(CrossEntropyTest, perfect_match_forward_test)
{
  using DataType = typename TypeParam::Type;

  uint64_t n_classes     = 4;
  uint64_t n_data_points = 8;

  TypeParam data1(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<uint64_t>{n_classes, n_data_points});

  std::vector<uint64_t> data = {1, 2, 3, 0, 3, 1, 0, 2};
  for (uint64_t i = 0; i < n_data_points; ++i)
  {
    for (uint64_t j = 0; j < n_classes; ++j)
    {
      if (data[i] == j)
      {
        data1.Set(j, i, DataType{1});
        data2.Set(j, i, DataType{1});
      }
      else
      {
        data1.Set(j, i, DataType{0});
        data2.Set(j, i, DataType{0});
      }
    }
  }

  fetch::ml::ops::CrossEntropyLoss<TypeParam> op;
  TypeParam                                   result({1, 1});
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, result);

  EXPECT_EQ(result(0, 0), typename TypeParam::Type(0));
}

TYPED_TEST(CrossEntropyTest, onehot_forward_test)
{
  uint64_t n_classes     = 3;
  uint64_t n_data_points = 2;

  TypeParam data1(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam gt(std::vector<uint64_t>{n_classes, n_data_points});

  data1 = TypeParam::FromString("0.05, 0.05, 0.9; 0.5, 0.2, 0.3");
  data1 = data1.Transpose();
  data2 = TypeParam::FromString("0.0, 1.0, 0; 1, 0, 0");
  data2 = data2.Transpose();

  fetch::ml::ops::CrossEntropyLoss<TypeParam> op;
  TypeParam                                   result({1, 1});
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, result);

  EXPECT_NEAR(static_cast<double>(result(0, 0)),
              static_cast<double>(3.6888794541) / static_cast<float>(n_data_points), 3e-7);
}

TYPED_TEST(CrossEntropyTest, onehot_forward_log_zero_test)
{
  using Type     = typename TypeParam::Type;
  using DataType = typename TypeParam::Type;

  uint64_t n_classes     = 3;
  uint64_t n_data_points = 2;

  TypeParam data1(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam gt(std::vector<uint64_t>{n_classes, n_data_points});

  data1 = TypeParam::FromString("0.1, 0.0, 0.9; 0.5, 0.0, 0.5");
  data1 = data1.Transpose();
  data2 = TypeParam::FromString("0.0, 1.0, 0; 1, 0, 0");
  data2 = data2.Transpose();

  fetch::ml::ops::CrossEntropyLoss<TypeParam> op;
  TypeParam                                   result({1, 1});
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, result);

  EXPECT_TRUE(result(0, 0) == fetch::math::numeric_inf<Type>());
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(CrossEntropyTest, binary_forward_test)
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  uint64_t n_classes     = 1;
  uint64_t n_data_points = 3;

  TypeParam data1(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam gt(std::vector<uint64_t>{n_classes, n_data_points});

  std::vector<double> input_vals{0.05, 0.1, 0.5};
  std::vector<double> targets{1.0, 0.0, 1.0};

  for (SizeType i = 0; i < n_data_points * n_classes; ++i)
  {
    data1.Set(SizeType{0}, i, fetch::math::AsType<DataType>(input_vals[i]));
    data2.Set(SizeType{0}, i, fetch::math::AsType<DataType>(targets[i]));
  }

  fetch::ml::ops::CrossEntropyLoss<TypeParam> op;
  TypeParam                                   result({1, 1});
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, result);

  ASSERT_FLOAT_EQ(static_cast<float>(result(0, 0)),
                  float(3.7942399698) / static_cast<float>(n_data_points));
}

TYPED_TEST(CrossEntropyTest, binary_backward_test)
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  uint64_t n_classes     = 1;
  uint64_t n_data_points = 3;

  TypeParam data1(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam gt(std::vector<uint64_t>{n_classes, n_data_points});

  // set gt data
  std::vector<double> gt_data{-20, 1.1111111111111111, -2.0000};
  for (SizeType i = 0; i < gt.size(); ++i)
  {
    gt.Set(SizeType{0}, i, fetch::math::AsType<DataType>(gt_data[i]));
  }
  gt = gt / static_cast<DataType>(n_data_points);

  std::vector<double> input_vals{0.05, 0.1, 0.5};
  std::vector<double> targets{1.0, 0.0, 1.0};

  for (SizeType i = 0; i < n_data_points * n_classes; ++i)
  {
    data1.Set(SizeType{0}, i, fetch::math::AsType<DataType>(input_vals[i]));
    data2.Set(SizeType{0}, i, fetch::math::AsType<DataType>(targets[i]));
  }

  TypeParam error_signal({1, 1});
  error_signal(0, 0) = DataType{1};

  fetch::ml::ops::CrossEntropyLoss<TypeParam> op;
  std::cout << op.Backward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)},
                           error_signal)
                   .at(0)
                   .ToString()
            << std::endl;
  EXPECT_TRUE(op.Backward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)},
                          error_signal)
                  .at(0)
                  .AllClose(gt, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(CrossEntropyTest, onehot_backward_test)
{
  using DataType = typename TypeParam::Type;

  uint64_t n_classes     = 3;
  uint64_t n_data_points = 2;

  TypeParam data1(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam gt(std::vector<uint64_t>{n_classes, n_data_points});

  // set gt data
  gt = TypeParam::FromString("0, -20.0000000000,  0; -2.0000000000,   0,   0");
  gt = gt.Transpose();
  gt = gt / static_cast<DataType>(n_data_points);

  data1 = TypeParam::FromString("0.05, 0.05, 0.9; 0.5, 0.2, 0.3");
  data1 = data1.Transpose();
  data2 = TypeParam::FromString("0.0, 1.0, 0; 1, 0, 0");
  data2 = data2.Transpose();

  TypeParam error_signal({1, 1});
  error_signal(0, 0) = DataType{1};

  fetch::ml::ops::CrossEntropyLoss<TypeParam> op;
  EXPECT_TRUE(op.Backward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)},
                          error_signal)
                  .at(0)
                  .AllClose(gt, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(CrossEntropyTest, saveparams_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::CrossEntropyLoss<TensorType>::SPType;
  using OpType     = fetch::ml::ops::CrossEntropyLoss<TensorType>;

  uint64_t n_classes     = 4;
  uint64_t n_data_points = 8;

  TypeParam data1(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<uint64_t>{n_classes, n_data_points});

  // set gt data
  std::vector<uint64_t> gt_data = {1, 2, 3, 0, 3, 1, 0, 2};
  for (uint64_t i = 0; i < n_data_points; ++i)
  {
    for (uint64_t j = 0; j < n_classes; ++j)
    {
      if (gt_data[i] == j)
      {
        data2.Set(j, i, DataType{1});
      }
      else
      {
        data2.Set(j, i, DataType{0});
      }
    }
  }

  // set softmax probabilities
  std::vector<double> logits{0.1, 0.8, 0.05, 0.05, 0.2, 0.5, 0.2, 0.1, 0.05, 0.05, 0.8,
                             0.1, 0.5, 0.1,  0.1,  0.3, 0.2, 0.3, 0.1, 0.4,  0.1,  0.7,
                             0.1, 0.1, 0.7,  0.1,  0.1, 0.1, 0.1, 0.1, 0.5,  0.3};

  uint64_t counter{0};
  for (uint64_t i{0}; i < n_data_points; ++i)
  {
    for (uint64_t j{0}; j < n_classes; ++j)
    {
      data1.Set(j, i, fetch::math::AsType<DataType>(logits[counter]));
      ++counter;
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

TYPED_TEST(CrossEntropyTest, saveparams_one_dimensional_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::CrossEntropyLoss<TensorType>::SPType;
  using OpType     = fetch::ml::ops::CrossEntropyLoss<TensorType>;

  uint64_t n_classes     = 4;
  uint64_t n_data_points = 8;

  TypeParam data1(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<uint64_t>{n_classes, n_data_points});
  TypeParam gt(std::vector<uint64_t>{n_classes, n_data_points});

  // set gt data
  gt = TypeParam::FromString("0, -20.0000000000,  0; -2.0000000000,   0,   0");
  gt = gt.Transpose();
  gt = gt / static_cast<DataType>(n_data_points);

  data1 = TypeParam::FromString("0.05, 0.05, 0.9; 0.5, 0.2, 0.3");
  data1 = data1.Transpose();
  data2 = TypeParam::FromString("0.0, 1.0, 0; 1, 0, 0");
  data2 = data2.Transpose();

  TypeParam error_signal({1, 1});
  error_signal(0, 0) = DataType{1};

  fetch::ml::ops::CrossEntropyLoss<TypeParam> op;

  // run op once to make sure caches etc. have been filled. Otherwise the test might be trivial!
  std::vector<TypeParam> gradients = op.Backward(
      {std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, error_signal);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

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
