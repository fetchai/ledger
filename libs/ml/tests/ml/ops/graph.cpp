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

#include "ml/graph.hpp"
#include "math/tensor.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/relu.hpp"
#include <gtest/gtest.h>

TEST(graph_test, node_placeholder)
{
  fetch::ml::Graph<fetch::math::Tensor<int>> g;
  g.AddNode<fetch::ml::ops::PlaceHolder<fetch::math::Tensor<int>>>("Input", {});

  std::shared_ptr<fetch::math::Tensor<int>> data = std::make_shared<fetch::math::Tensor<int>>(8);
  std::shared_ptr<fetch::math::Tensor<int>> gt   = std::make_shared<fetch::math::Tensor<int>>(8);
  std::size_t                                i(0);
  for (int e : {1, 2, 3, 4, 5, 6, 7, 8})
  {
    data->Set(i, e);
    gt->Set(i, e);
    i++;
  }

  g.SetInput("Input", data);
  std::shared_ptr<fetch::math::Tensor<int>> prediction = g.Evaluate("Input");

  // test correct values
  ASSERT_TRUE(prediction->AllClose(*gt));
}

TEST(graph_test, node_relu)
{
  fetch::ml::Graph<fetch::math::Tensor<int>> g;
  g.AddNode<fetch::ml::ops::PlaceHolder<fetch::math::Tensor<int>>>("Input", {});
  g.AddNode<fetch::ml::ops::ReluLayer<fetch::math::Tensor<int>>>("Relu", {"Input"});

  std::shared_ptr<fetch::math::Tensor<int>> data =
      std::make_shared<fetch::math::Tensor<int>>(std::vector<std::size_t>({4, 4}));
  std::shared_ptr<fetch::math::Tensor<int>> gt =
      std::make_shared<fetch::math::Tensor<int>>(std::vector<std::size_t>({4, 4}));
  std::vector<int> dataValues({0, -1, 2, -3, 4, -5, 6, -7, 8, -9, 10, -11, 12, -13, 14, -15, 16});
  std::vector<int> gtValues({0, 0, 2, 0, 4, 0, 6, 0, 8, 0, 10, 0, 12, 0, 14, 0, 16});
  for (std::size_t i(0); i < 4; ++i)
  {
    for (std::size_t j(0); j < 4; ++j)
    {
      data->Set(std::vector<std::size_t>({i, j}), dataValues[i * 4 + j]);
      gt->Set(std::vector<std::size_t>({i, j}), gtValues[i * 4 + j]);
    }
  }

  g.SetInput("Input", data);
  std::shared_ptr<fetch::math::Tensor<int>> prediction = g.Evaluate("Relu");

  // test correct values
  ASSERT_TRUE(prediction->AllClose(*gt));
}
