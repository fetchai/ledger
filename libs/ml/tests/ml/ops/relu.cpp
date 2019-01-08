
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

#include "ml/ops/relu.hpp"
#include "math/ndarray.hpp"
#include <gtest/gtest.h>

TEST(relu_test, forward_all_positive_integer_test)
{
  std::shared_ptr<fetch::math::NDArray<int>> data = std::make_shared<fetch::math::NDArray<int>>(8);
  std::shared_ptr<fetch::math::NDArray<int>> gt   = std::make_shared<fetch::math::NDArray<int>>(8);
  size_t                                     i(0);
  for (int e : {1, 2, 3, 4, 5, 6, 7, 8})
  {
    data->Set(i, e);
    gt->Set(i, e);
    i++;
  }
  fetch::ml::ops::ReluLayer<fetch::math::NDArray<int>> op;
  std::shared_ptr<fetch::math::NDArray<int>>           prediction = op.Forward({data});

  // test correct values
  ASSERT_TRUE(prediction->AllClose(*gt));
}

TEST(relu_test, forward_all_negative_integer_test)
{
  std::shared_ptr<fetch::math::NDArray<int>> data = std::make_shared<fetch::math::NDArray<int>>(8);
  std::shared_ptr<fetch::math::NDArray<int>> gt   = std::make_shared<fetch::math::NDArray<int>>(8);
  size_t                                     i(0);
  for (int e : {-1, -2, -3, -4, -5, -6, -7, -8})
  {
    data->Set(i, e);
    gt->Set(i, 0);
    i++;
  }
  fetch::ml::ops::ReluLayer<fetch::math::NDArray<int>> op;
  std::shared_ptr<fetch::math::NDArray<int>>           prediction = op.Forward({data});

  // test correct values
  ASSERT_TRUE(prediction->AllClose(*gt));
}

TEST(relu_test, forward_mixed_integer_test)
{
  std::shared_ptr<fetch::math::NDArray<int>> data = std::make_shared<fetch::math::NDArray<int>>(8);
  std::shared_ptr<fetch::math::NDArray<int>> gt   = std::make_shared<fetch::math::NDArray<int>>(8);
  std::vector<int>                           dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int>                           gtInput({1, 0, 3, 0, 5, 0, 7, 0});
  for (size_t i(0); i < 8; ++i)
  {
    data->Set(i, dataInput[i]);
    gt->Set(i, gtInput[i]);
  }
  fetch::ml::ops::ReluLayer<fetch::math::NDArray<int>> op;
  std::shared_ptr<fetch::math::NDArray<int>>           prediction = op.Forward({data});

  // test correct values
  ASSERT_TRUE(prediction->AllClose(*gt));
}

TEST(relu_test, backward_mixed_integer_test)
{
  std::shared_ptr<fetch::math::NDArray<int>> data  = std::make_shared<fetch::math::NDArray<int>>(8);
  std::shared_ptr<fetch::math::NDArray<int>> error = std::make_shared<fetch::math::NDArray<int>>(8);
  std::shared_ptr<fetch::math::NDArray<int>> gt    = std::make_shared<fetch::math::NDArray<int>>(8);
  std::vector<int>                           dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int>                           errorInput({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int>                           gtInput({-1, 0, 3, 0, -8, 0, -21, 0});
  for (size_t i(0); i < 8; ++i)
  {
    data->Set(i, dataInput[i]);
    error->Set(i, errorInput[i]);
    gt->Set(i, gtInput[i]);
  }
  fetch::ml::ops::ReluLayer<fetch::math::NDArray<int>>    op;
  std::vector<std::shared_ptr<fetch::math::NDArray<int>>> prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(prediction[0]->AllClose(*gt));
}

TEST(relu_test, forward_mixed_float_test)
{
  std::shared_ptr<fetch::math::NDArray<float>> data =
      std::make_shared<fetch::math::NDArray<float>>(8);
  std::shared_ptr<fetch::math::NDArray<float>> gt =
      std::make_shared<fetch::math::NDArray<float>>(8);
  std::vector<float> dataInput(
      {1.1f, -2.22f, 3.333f, -4.4444f, 5.55555f, -6.666666f, 7.7777777f, -8.88888888f});
  std::vector<float> gtInput({1.1f, 0.0f, 3.333f, 0.0f, 5.55555f, 0.0f, 7.7777777f, 0.0f});
  for (size_t i(0); i < 8; ++i)
  {
    data->Set(i, dataInput[i]);
    gt->Set(i, gtInput[i]);
  }
  fetch::ml::ops::ReluLayer<fetch::math::NDArray<float>> op;
  std::shared_ptr<fetch::math::NDArray<float>>           prediction = op.Forward({data});

  // test correct values
  ASSERT_TRUE(prediction->AllClose(*gt));
}
