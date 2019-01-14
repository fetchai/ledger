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

#include "ml/ops/mean_square_error.hpp"
#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"
#include <gtest/gtest.h>

TEST(mean_square_error_test, perfect_match_forward_matrix_test)
{
  std::shared_ptr<fetch::math::linalg::Matrix<int>> data1 =
      std::make_shared<fetch::math::linalg::Matrix<int>>(8);
  std::shared_ptr<fetch::math::linalg::Matrix<int>> data2 =
      std::make_shared<fetch::math::linalg::Matrix<int>>(8);
  size_t i(0);
  for (int e : {1, -2, 3, -4, 5, -6, 7, -8})
  {
    data1->Set(i, e);
    data2->Set(i, e);
    i++;
  }

  fetch::ml::ops::MeanSquareErrorLayer<fetch::math::linalg::Matrix<int>> op;
  EXPECT_EQ(op.Forward({data1, data2}), 0.0f);
}

TEST(mean_square_error_test, perfect_match_forward_ndarray_test)
{
  std::shared_ptr<fetch::math::NDArray<int>> data1 = std::make_shared<fetch::math::NDArray<int>>(8);
  std::shared_ptr<fetch::math::NDArray<int>> data2 = std::make_shared<fetch::math::NDArray<int>>(8);
  size_t                                     i(0);
  for (int e : {1, -2, 3, -4, 5, -6, 7, -8})
  {
    data1->Set(i, e);
    data2->Set(i, e);
    i++;
  }

  // TODO(private 489) fetch::math::MeanSquareError does not compile using NDArray<int>
  // fetch::ml::ops::MeanSquareErrorLayer<fetch::math::NDArray<int>> op;
  // EXPECT_EQ(op.Forward({data1, data2}), 0.0f);
}

TEST(mean_square_error_test, one_dimensional_forward_matrix_test)
{
  std::shared_ptr<fetch::math::linalg::Matrix<float>> data1 =
      std::make_shared<fetch::math::linalg::Matrix<float>>(8);
  std::shared_ptr<fetch::math::linalg::Matrix<float>> data2 =
      std::make_shared<fetch::math::linalg::Matrix<float>>(8);
  size_t i(0);
  for (float e : {1.1f, -2.2f, 3.3f, -4.4f, 5.5f, -6.6f, 7.7f, -8.8f})
  {
    data1->Set(i, e);
    i++;
  }
  i = 0;
  for (float e : {1.1f, 2.2f, 7.7f, 6.6f, 0.0f, -6.6f, 7.7f, -9.9f})
  {
    data2->Set(i, e);
    i++;
  }

  fetch::ml::ops::MeanSquareErrorLayer<fetch::math::linalg::Matrix<float>> op;
  EXPECT_EQ(op.Forward({data1, data2}), 191.18f / 8.0f / 2.0f);
  // fetch::math::MeanSquareError divided sum by number of element (ie 8 in this case)
  // and then further didivde by do (cf issue 343)
}

TEST(mean_square_error_test, one_dimensional_backward_matrix_test)
{
  std::shared_ptr<fetch::math::linalg::Matrix<float>> data1 =
      std::make_shared<fetch::math::linalg::Matrix<float>>(8);
  std::shared_ptr<fetch::math::linalg::Matrix<float>> data2 =
      std::make_shared<fetch::math::linalg::Matrix<float>>(8);
  std::shared_ptr<fetch::math::linalg::Matrix<float>> gt =
      std::make_shared<fetch::math::linalg::Matrix<float>>(8);
  size_t i(0);
  for (float e : {1.1f, -2.2f, 3.3f, -4.4f, 5.5f, -6.6f, 7.7f, -8.8f})
  {
    data1->Set(i, e);
    i++;
  }
  i = 0;
  for (float e : {1.1f, 2.2f, 7.7f, 6.6f, 0.0f, -6.6f, 7.7f, -9.9f})
  {
    data2->Set(i, e);
    i++;
  }
  i = 0;
  for (float e : {0.0f, -4.4f, -4.4f, -11.0f, 5.5f, 0.0f, 0.0f, 1.1f})
  {
    gt->Set(i, e);
    i++;
  }

  fetch::ml::ops::MeanSquareErrorLayer<fetch::math::linalg::Matrix<float>> op;
  EXPECT_TRUE(op.Backward({data1, data2})->AllClose(*gt));
}
