//------------------------------------------------------------------------------
//
//   Copyright 2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http =//www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <gtest/gtest.h>
#include "math/ndarray.hpp"
#include "math/linalg/matrix.hpp"
#include "ml/ops/matrixMultiply.hpp"

TEST(matrixMultiply_test, forward_test)
{
  std::shared_ptr<fetch::math::linalg::Matrix<double>> a  = std::make_shared<fetch::math::linalg::Matrix<double>>(std::vector<size_t>({1, 5}));
  std::shared_ptr<fetch::math::linalg::Matrix<double>> b  = std::make_shared<fetch::math::linalg::Matrix<double>>(std::vector<size_t>({5, 4}));
  std::shared_ptr<fetch::math::linalg::Matrix<double>> gt = std::make_shared<fetch::math::linalg::Matrix<double>>(std::vector<size_t>({1, 4}));

  std::vector<double> data({1, 2, -3, 4, 5});
  std::vector<double> weights({-11, 12, 13, 14,
			       21, 22, 23, 24,
			       31, 32, 33, 34,
			       41, 42, 43, 44,
			       51, 52, 53, 54});
  std::vector<double> results({357, 388, 397, 406});
  
  for (size_t i(0) ; i < data.size() ; ++i)
    a->Set(0, i, data[i]);
  for (size_t i(0) ; i < 5 ; ++i)
    for (size_t j(0) ; j < 4 ; ++j)
      b->Set(i, j, weights[i * 4 + j]);
  for (size_t i(0) ; i < results.size() ; ++i)
    gt->Set(0, i, results[i]);

    
  fetch::ml::ops::MatrixMultiply<fetch::math::linalg::Matrix<double>> op;
  std::shared_ptr<fetch::math::linalg::Matrix<double>> prediction = op.forward({a, b});

  // test correct values
  ASSERT_TRUE(prediction->shape().size() == 2);
  ASSERT_TRUE(prediction->shape()[0] == 1);
  ASSERT_TRUE(prediction->shape()[1] == 4);
  ASSERT_TRUE(prediction->AllClose(*gt));
}

TEST(matrixMultiply_test, backward_test)
{
  std::shared_ptr<fetch::math::linalg::Matrix<double>> a  = std::make_shared<fetch::math::linalg::Matrix<double>>(std::vector<size_t>({1, 5}));
  std::shared_ptr<fetch::math::linalg::Matrix<double>> b  = std::make_shared<fetch::math::linalg::Matrix<double>>(std::vector<size_t>({5, 4}));
  std::shared_ptr<fetch::math::linalg::Matrix<double>> e  = std::make_shared<fetch::math::linalg::Matrix<double>>(std::vector<size_t>({1, 4}));
  std::shared_ptr<fetch::math::linalg::Matrix<double>> ig = std::make_shared<fetch::math::linalg::Matrix<double>>(std::vector<size_t>({1, 5}));
  std::shared_ptr<fetch::math::linalg::Matrix<double>> wg = std::make_shared<fetch::math::linalg::Matrix<double>>(std::vector<size_t>({5, 4}));


  std::vector<double> data({1, 2, -3, 4, 5});
  std::vector<double> weights({-11, 12, 13, 14,
			       21, 22, 23, 24,
			       31, 32, 33, 34,
			       41, 42, 43, 44,
			       51, 52, 53, 54});
  std::vector<double> errorSignal({1, 2, 3, -4});
  std::vector<double> inputGrad({-4, 38, 58, 78, 98});
  std::vector<double> weightsGrad({1, 2, 3, -4,
				   2, 4, 6, -8,
				   -3, -6, -9, 12,
				   4, 8, 12, -16,
				   5, 10, 15, -20});

  for (size_t i(0) ; i < data.size() ; ++i)
    a->Set(0, i, data[i]);
  for (size_t i(0) ; i < 5 ; ++i)
    for (size_t j(0) ; j < 4 ; ++j)
      b->Set(i, j, weights[i * 4 + j]);
  for (size_t i(0) ; i < errorSignal.size() ; ++i)
    e->Set(0, i, errorSignal[i]);
  for (size_t i(0) ; i < inputGrad.size() ; ++i)
    ig->Set(0, i, inputGrad[i]);
  for (size_t i(0) ; i < 5 ; ++i)
    for (size_t j(0) ; j < 4 ; ++j)
      wg->Set(i, j, weightsGrad[i * 4 + j]);
  
  fetch::ml::ops::MatrixMultiply<fetch::math::linalg::Matrix<double>> op;
  std::vector<std::shared_ptr<fetch::math::linalg::Matrix<double>>> backpropagatedSignals = op.backward({a, b}, e);

  // test correct shapes
  ASSERT_EQ(backpropagatedSignals.size(), 2);
  ASSERT_EQ(backpropagatedSignals[0]->shape().size(), 2);
  EXPECT_EQ(backpropagatedSignals[0]->shape()[0], 1);
  EXPECT_EQ(backpropagatedSignals[0]->shape()[1], 5);
  ASSERT_EQ(backpropagatedSignals[1]->shape().size(), 2);
  EXPECT_EQ(backpropagatedSignals[1]->shape()[0], 5);
  ASSERT_EQ(backpropagatedSignals[1]->shape()[1], 4);

  // test correct values
  EXPECT_TRUE(backpropagatedSignals[0]->AllClose(*ig));
  EXPECT_TRUE(backpropagatedSignals[1]->AllClose(*wg));
}
