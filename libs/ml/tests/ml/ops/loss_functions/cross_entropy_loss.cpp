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
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class CrossEntropyTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(CrossEntropyTest, MyTypes);

TYPED_TEST(CrossEntropyTest, perfect_match_forward_test)
{
  using DataType = typename TypeParam::Type;

  std::uint64_t n_classes     = 4;
  std::uint64_t n_data_points = 8;

  TypeParam data1(std::vector<std::uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<std::uint64_t>{n_classes, n_data_points});

  std::vector<std::uint64_t> data = {1, 2, 3, 0, 3, 1, 0, 2};
  for (std::uint64_t i = 0; i < n_data_points; ++i)
  {
    for (std::uint64_t j = 0; j < n_classes; ++j)
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
  TypeParam                                        result({1, 1});
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, result);

  EXPECT_EQ(result(0, 0), typename TypeParam::Type(0));
}

TYPED_TEST(CrossEntropyTest, one_dimensional_forward_test)
{
  std::uint64_t n_classes     = 4;
  std::uint64_t n_data_points = 8;

  TypeParam data1(std::vector<std::uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<std::uint64_t>{n_classes, n_data_points});

  // set gt data
  std::vector<std::uint64_t> gt_data = {1, 2, 3, 0, 3, 1, 0, 2};
  for (std::uint64_t i = 0; i < n_data_points; ++i)
  {
    for (std::uint64_t j = 0; j < n_classes; ++j)
    {
      if (gt_data[i] == j)
      {
        data2.Set(j, i, typename TypeParam::Type(1));
      }
      else
      {
        data2.Set(j, i, typename TypeParam::Type(0));
      }
    }
  }

  // set softmax probabilities
  std::vector<double> logits{0.1, 0.8, 0.05, 0.05, 0.2, 0.5, 0.2, 0.1, 0.05, 0.05, 0.8,
                             0.1, 0.5, 0.1,  0.1,  0.3, 0.2, 0.3, 0.1, 0.4,  0.1,  0.7,
                             0.1, 0.1, 0.7,  0.1,  0.1, 0.1, 0.1, 0.1, 0.5,  0.3};

  std::uint64_t counter{0};
  for (std::uint64_t i{0}; i < n_data_points; ++i)
  {
    for (std::uint64_t j{0}; j < n_classes; ++j)
    {
      data1.Set(j, i, typename TypeParam::Type(logits[counter]));
      ++counter;
    }
  }

  fetch::ml::ops::CrossEntropyLoss<TypeParam> op;
  TypeParam                                        result({1, 1});
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, result);

  EXPECT_NEAR(static_cast<double>(result(0, 0)), static_cast<double>(0.893887639), 3e-7);
}

TYPED_TEST(CrossEntropyTest, non_one_hot_forward_test)
{
  std::uint64_t n_classes     = 1;
  std::uint64_t n_data_points = 4;

  TypeParam data1(std::vector<std::uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<std::uint64_t>{n_classes, n_data_points});

  // set gt data
  std::vector<std::uint64_t> gt_data = {0, 0, 0, 1};
  for (std::uint64_t i = 0; i < n_data_points; ++i)
  {
    for (std::uint64_t j = 0; j < n_classes; ++j)
    {
      if (gt_data[i] == j)
      {
        data2.Set(j, i, typename TypeParam::Type(1));
      }
      else
      {
        data2.Set(j, i, typename TypeParam::Type(0));
      }
    }
  }

  // set softmax probabilities
  std::vector<double> logits{0.01, 0.05, 0.50, 0.9};

  for (std::uint64_t i = 0; i < n_data_points * n_classes; ++i)
  {
    data1.Set(0, i, typename TypeParam::Type(logits[i]));
  }

  fetch::ml::ops::CrossEntropyLoss<TypeParam> op;
  TypeParam                                        result({1, 1});
  op.Forward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)}, result);

  ASSERT_FLOAT_EQ(static_cast<float>(result(0, 0)), float(2.6491587));
}

TYPED_TEST(CrossEntropyTest, trivial_one_dimensional_backward_test)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  std::uint64_t n_classes     = 3;
  std::uint64_t n_data_points = 1;

  TypeParam data1(std::vector<std::uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<std::uint64_t>{n_classes, n_data_points});
  TypeParam gt(std::vector<std::uint64_t>{n_classes, n_data_points});

  // set gt data
  std::vector<double> gt_data{-0., -9.3890561, -0.};
  for (SizeType i = 0; i < gt.size(); ++i)
  {
    gt.Set(i, SizeType{0}, typename TypeParam::Type(gt_data[i]));
  }

  std::vector<double> unscaled_vals{-1.0, -1.0, 1.0};
  std::vector<double> targets{0.0, 1.0, 0.0};

  for (SizeType i = 0; i < n_data_points * n_classes; ++i)
  {
    data1.Set(i, SizeType{0}, typename TypeParam::Type(unscaled_vals[i]));
    data2.Set(i, SizeType{0}, typename TypeParam::Type(targets[i]));
  }

  TypeParam error_signal({1, 1});
  error_signal(0, 0) = DataType{1};

  fetch::ml::ops::CrossEntropyLoss<TypeParam> op;
  EXPECT_TRUE(op.Backward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)},
                          error_signal)
                  .at(0)
                  .AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(CrossEntropyTest, one_dimensional_backward_test)
{
  using DataType = typename TypeParam::Type;

  std::uint64_t n_classes     = 4;
  std::uint64_t n_data_points = 8;

  TypeParam data1(std::vector<std::uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<std::uint64_t>{n_classes, n_data_points});
  TypeParam gt(std::vector<std::uint64_t>{n_classes, n_data_points});

  // set gt data
  std::vector<double> gt_data{-0., -0.244132, -0., -0.,       -0., -0.315196, -0.,       -0., -0.,
                              -0., -0.244132, -0., -0.315937, -0., -0.,       -0.,       -0., -0.,
                              -0., -0.346439, -0., -0.264643, -0., -0.,       -0.264643, -0., -0.,
                              -0., -0.,       -0., -0.315937, -0.};

  std::uint64_t counter{0};
  for (std::uint64_t i = 0; i < n_data_points; ++i)
  {
    for (std::uint64_t j = 0; j < n_classes; ++j)
    {
      gt.Set(j, i, typename TypeParam::Type(gt_data[counter]));
      ++counter;
    }
  }

  std::vector<double> unscaled_vals{0.1, 0.8, 0.05, 0.05, 0.2, 0.5, 0.2, 0.1, 0.05, 0.05, 0.8,
                                    0.1, 0.5, 0.1,  0.1,  0.3, 0.2, 0.3, 0.1, 0.4,  0.1,  0.7,
                                    0.1, 0.1, 0.7,  0.1,  0.1, 0.1, 0.1, 0.1, 0.5,  0.3};
  std::vector<double> target{0.0, 0.1, 0.00, 0.00, 0.0, 0.1, 0.0, 0.0, 0.0, 0.0, 0.1,
                             0.0, 0.1, 0.0,  0.0,  0.0, 0.0, 0.0, 0.0, 0.1, 0.0, 0.1,
                             0.0, 0.0, 0.1,  0.0,  0.0, 0.0, 0.0, 0.0, 0.1, 0.0};

  counter = 0;
  for (std::uint64_t i = 0; i < n_data_points; ++i)
  {
    for (std::uint64_t j = 0; j < n_classes; ++j)
    {
      data1.Set(j, i, typename TypeParam::Type(unscaled_vals[counter]));
      data2.Set(j, i, typename TypeParam::Type(target[counter]));
      ++counter;
    }
  }

  TypeParam error_signal({1, 1});
  error_signal(0, 0) = DataType{1};

  fetch::ml::ops::CrossEntropyLoss<TypeParam> op;
  EXPECT_TRUE(op.Backward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)},
                          error_signal)
                  .at(0)
                  .AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(CrossEntropyTest, non_one_hot_dimensional_backward_test)
{
  std::uint64_t n_classes     = 1;
  std::uint64_t n_data_points = 8;

  TypeParam data1(std::vector<std::uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<std::uint64_t>{n_classes, n_data_points});
  TypeParam gt(std::vector<std::uint64_t>{n_classes, n_data_points});

  // set gt data
  std::vector<double> gt_data{0.0524979, -0.24802, -0.0243751, 0., 0., 26, 1e+9, 0};
  for (std::uint64_t i = 0; i < gt.size(); ++i)
  {
    gt.Set(0, i, typename TypeParam::Type(gt_data[i]));
  }

  // theoretically these needn't lie between 0 and 1
  std::vector<double> unscaled_vals{0.1, 0.8, -0.05, 100000, 123456, -26, 999999999, 9999999};
  std::vector<double> target{0.0, 1.0, 0., 1.0, 1.0, 1., 0.0, 1.0};
  for (std::uint64_t i = 0; i < n_data_points * n_classes; ++i)
  {
    data1.Set(0, i, typename TypeParam::Type(unscaled_vals[i]));
    data2.Set(0, i, typename TypeParam::Type(target[i]));
  }

  TypeParam error_signal({1, 1});
  error_signal(0, 0) = typename TypeParam::Type{1};

  fetch::ml::ops::CrossEntropyLoss<TypeParam> op;
  EXPECT_TRUE(op.Backward({std::make_shared<TypeParam>(data1), std::make_shared<TypeParam>(data2)},
                          error_signal)
                  .at(0)
                  .AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(CrossEntropyTest, saveparams_test)
{
  using ArrayType = TypeParam;
  using DataType  = typename TypeParam::Type;
  using SPType    = typename fetch::ml::ops::CrossEntropyLoss<ArrayType>::SPType;
  using OpType    = typename fetch::ml::ops::CrossEntropyLoss<ArrayType>;

  std::uint64_t n_classes     = 4;
  std::uint64_t n_data_points = 8;

  TypeParam data1(std::vector<std::uint64_t>{n_classes, n_data_points});
  TypeParam data2(std::vector<std::uint64_t>{n_classes, n_data_points});

  // set gt data
  std::vector<std::uint64_t> gt_data = {1, 2, 3, 0, 3, 1, 0, 2};
  for (std::uint64_t i = 0; i < n_data_points; ++i)
  {
    for (std::uint64_t j = 0; j < n_classes; ++j)
    {
      if (gt_data[i] == j)
      {
        data2.Set(j, i, DataType(1));
      }
      else
      {
        data2.Set(j, i, DataType(0));
      }
    }
  }

  // set softmax probabilities
  std::vector<double> logits{0.1, 0.8, 0.05, 0.05, 0.2, 0.5, 0.2, 0.1, 0.05, 0.05, 0.8,
                             0.1, 0.5, 0.1,  0.1,  0.3, 0.2, 0.3, 0.1, 0.4,  0.1,  0.7,
                             0.1, 0.1, 0.7,  0.1,  0.1, 0.1, 0.1, 0.1, 0.5,  0.3};

  std::uint64_t counter{0};
  for (std::uint64_t i{0}; i < n_data_points; ++i)
  {
    for (std::uint64_t j{0}; j < n_classes; ++j)
    {
      data1.Set(j, i, DataType(logits[counter]));
      ++counter;
    }
  }

  OpType    op;
  TypeParam result({1, 1});
  op.Forward({data1, data2}, result);

  // extract saveparams
  std::shared_ptr<fetch::ml::SaveableParamsInterface> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::ByteArrayBuffer b;
  b << *dsp;

  // make another prediction with the original graph
  op.Forward({data1, data2}, result);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TypeParam new_result({1, 1});
  op.Forward({data1, data2}, new_result);

  // test correct values
  EXPECT_NEAR(static_cast<double>(result(0, 0)), static_cast<double>(new_result(0, 0)),
              static_cast<double>(fetch::math::function_tolerance<DataType>()));
}
