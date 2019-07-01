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

#include "math/ml/loss_functions/cross_entropy.hpp"
#include "math/tensor.hpp"

#include "gtest/gtest.h"

template <typename T>
class CrossEntropyTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(CrossEntropyTest, MyTypes);

TYPED_TEST(CrossEntropyTest, perfect_match_test)
{
  using SizeType     = fetch::math::SizeType;
  SizeType n_classes = 4;
  SizeType n_data    = 8;

  TypeParam gt_array = TypeParam{{n_data, n_classes}};

  // set gt data
  std::vector<std::uint64_t> gt_data = {1, 2, 3, 0, 3, 1, 0, 2};
  for (SizeType i = 0; i < n_data; ++i)
  {
    for (SizeType j = 0; j < n_classes; ++j)
    {
      if (gt_data[i] == j)
      {
        gt_array(i, j) = typename TypeParam::Type(1);
      }
      else
      {
        gt_array(i, j) = typename TypeParam::Type(0);
      }
    }
  }
  TypeParam test_array = gt_array.Copy();

  // initialise to non-zero just to avoid correct value at initialisation
  typename TypeParam::Type score(100);
  score = fetch::math::CrossEntropyLoss(test_array, gt_array);

  // test correct values
  ASSERT_NEAR(double(score), double(0.), double(1.0e-5f));
}

TYPED_TEST(CrossEntropyTest, value_test)
{
  using SizeType                         = fetch::math::SizeType;
  typename TypeParam::SizeType n_classes = 4;
  typename TypeParam::SizeType n_data    = 8;

  TypeParam test_array = TypeParam{{n_classes, n_data}};
  TypeParam gt_array   = TypeParam{{n_classes, n_data}};

  // set gt data
  std::vector<std::uint64_t> gt_data = {1, 2, 3, 0, 3, 1, 0, 2};
  for (typename TypeParam::SizeType i = 0; i < n_data; ++i)
  {
    for (typename TypeParam::SizeType j = 0; j < n_classes; ++j)
    {
      if (gt_data[i] == j)
      {
        gt_array(j, i) = typename TypeParam::Type(1);
      }
      else
      {
        gt_array(j, i) = typename TypeParam::Type(0);
      }
    }
  }

  // set softmax probabilities
  std::vector<double> logits{0.1, 0.8, 0.05, 0.05, 0.2, 0.5, 0.2, 0.1, 0.05, 0.05, 0.8,
                             0.1, 0.5, 0.1,  0.1,  0.3, 0.2, 0.3, 0.1, 0.4,  0.1,  0.7,
                             0.1, 0.1, 0.7,  0.1,  0.1, 0.1, 0.1, 0.1, 0.5,  0.3};

  SizeType idx{0};
  for (SizeType i{0}; i < n_data; ++i)
  {
    for (SizeType j{0}; j < n_classes; ++j)
    {
      test_array(j, i) = typename TypeParam::Type(logits.at(idx));
      idx++;
    }
  }

  // initialise to non-zero just to avoid correct value at initialisation
  typename TypeParam::Type score(0);
  score = fetch::math::CrossEntropyLoss(test_array, gt_array);

  // test correct values
  ASSERT_NEAR(double(score), double(0.893887639), double(1.0e-5f));
}
