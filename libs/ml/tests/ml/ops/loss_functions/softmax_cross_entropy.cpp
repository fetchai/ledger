////------------------------------------------------------------------------------
////
////   Copyright 2018-2019 Fetch.AI Limited
////
////   Licensed under the Apache License, Version 2.0 (the "License");
////   you may not use this file except in compliance with the License.
////   You may obtain a copy of the License at
////
////       http://www.apache.org/licenses/LICENSE-2.0
////
////   Unless required by applicable law or agreed to in writing, software
////   distributed under the License is distributed on an "AS IS" BASIS,
////   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
////   See the License for the specific language governing permissions and
////   limitations under the License.
////
////------------------------------------------------------------------------------
//
//#include "ml/ops/loss_functions/cross_entropy.hpp"
//#include "core/fixed_point/fixed_point.hpp"
//#include "math/tensor.hpp"
//#include <gtest/gtest.h>
//
// template <typename T>
// class CrossEntropyTest : public ::testing::Test
//{
//};
//
// using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
//                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
// TYPED_TEST_CASE(CrossEntropyTest, MyTypes);
//
// TYPED_TEST(CrossEntropyTest, perfect_match_forward_test)
//{
//  std::uint64_t n_classes     = 4;
//  std::uint64_t n_data_points = 8;
//
//  TypeParam data1(std::vector<std::uint64_t>{n_data_points, n_classes});
//  TypeParam data2(std::vector<std::uint64_t>{n_data_points, n_classes});
//
//  std::vector<std::uint64_t> data = {1, 2, 3, 0, 3, 1, 0, 2};
//  for (std::uint64_t i = 0; i < n_data_points; ++i)
//  {
//    for (std::uint64_t j = 0; j < n_classes; ++j)
//    {
//      if (data[i] == j)
//      {
//        data1.Set({i, j}, typename TypeParam::Type(1));
//        data2.Set({i, j}, typename TypeParam::Type(1));
//      }
//      else
//      {
//        data1.Set({i, j}, typename TypeParam::Type(0));
//        data2.Set({i, j}, typename TypeParam::Type(0));
//      }
//    }
//  }
//
//  fetch::ml::ops::CrossEntropy<TypeParam> op;
//  EXPECT_EQ(op.Forward({data1, data2}), typename TypeParam::Type(0));
//}
//
// TYPED_TEST(CrossEntropyTest, one_dimensional_forward_test)
//{
//  std::uint64_t n_classes     = 4;
//  std::uint64_t n_data_points = 8;
//
//  TypeParam data1(std::vector<std::uint64_t>{n_data_points, n_classes});
//  TypeParam data2(std::vector<std::uint64_t>{n_data_points, n_classes});
//
//  // set gt data
//  std::vector<std::uint64_t> gt_data = {1, 2, 3, 0, 3, 1, 0, 2};
//  for (std::uint64_t i = 0; i < n_data_points; ++i)
//  {
//    for (std::uint64_t j = 0; j < n_classes; ++j)
//    {
//      if (gt_data[i] == j)
//      {
//        data2.Set({i, j}, typename TypeParam::Type(1));
//      }
//      else
//      {
//        data2.Set({i, j}, typename TypeParam::Type(0));
//      }
//    }
//  }
//
//  // set softmax probabilities
//  std::vector<double> logits{0.1, 0.8, 0.05, 0.05, 0.2, 0.5, 0.2, 0.1, 0.05, 0.05, 0.8,
//                             0.1, 0.5, 0.1,  0.1,  0.3, 0.2, 0.3, 0.1, 0.4,  0.1,  0.7,
//                             0.1, 0.1, 0.7,  0.1,  0.1, 0.1, 0.1, 0.1, 0.5,  0.3};
//
//  for (std::uint64_t i = 0; i < n_data_points * n_classes; ++i)
//  {
//    data1.Set(i, typename TypeParam::Type(logits[i]));
//  }
//
//  fetch::ml::ops::CrossEntropy<TypeParam> op;
//  ASSERT_FLOAT_EQ(float(op.Forward({data1, data2})), 1.78777539730072021484375);
//}
//
// TYPED_TEST(CrossEntropyTest, one_dimensional_backward_test)
//{
//  std::uint64_t n_classes     = 4;
//  std::uint64_t n_data_points = 8;
//
//  TypeParam data1(std::vector<std::uint64_t>{n_classes, n_data_points});
//  TypeParam data2(std::vector<std::uint64_t>{n_classes, n_data_points});
//  TypeParam gt(std::vector<std::uint64_t>{n_classes, n_data_points});
//
//  // set gt data
//  std::vector<double> gt_data{
//      0.003125,  0.021875,  0.0015625, 0.0015625, 0.00625,  0.0125,   0.00625,  0.003125,
//      0.0015625, 0.0015625, 0.021875,  0.003125,  0.0125,   0.003125, 0.003125, 0.009375,
//      0.00625,   0.009375,  0.003125,  0.009375,  0.003125, 0.01875,  0.003125, 0.003125,
//      0.01875,   0.003125,  0.003125,  0.003125,  0.003125, 0.003125, 0.0125,   0.009375};
//  for (std::uint64_t i = 0; i < gt.size(); ++i)
//  {
//    gt.Set(i, typename TypeParam::Type(gt_data[i]));
//  }
//
//  // set softmax probabilities
//  std::vector<double> logits{0.1, 0.8, 0.05, 0.05, 0.2, 0.5, 0.2, 0.1, 0.05, 0.05, 0.8,
//                             0.1, 0.5, 0.1,  0.1,  0.3, 0.2, 0.3, 0.1, 0.4,  0.1,  0.7,
//                             0.1, 0.1, 0.7,  0.1,  0.1, 0.1, 0.1, 0.1, 0.5,  0.3};
//  std::vector<double> err{0.0, 0.1, 0.00, 0.00, 0.0, 0.1, 0.0, 0.0, 0.0, 0.0, 0.1,
//                          0.0, 0.1, 0.0,  0.0,  0.0, 0.0, 0.0, 0.0, 0.1, 0.0, 0.1,
//                          0.0, 0.0, 0.1,  0.0,  0.0, 0.0, 0.0, 0.0, 0.1, 0.0};
//  for (std::uint64_t i = 0; i < n_data_points * n_classes; ++i)
//  {
//    data1.Set(i, typename TypeParam::Type(logits[i]));
//    data2.Set(i, typename TypeParam::Type(err[i]));
//  }
//
//  fetch::ml::ops::CrossEntropy<TypeParam> op;
//  EXPECT_TRUE(op.Backward({data1, data2})
//                  .AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
//}
