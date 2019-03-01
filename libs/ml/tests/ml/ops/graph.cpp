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
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/placeholder.hpp"
#include <gtest/gtest.h>

using ArrayType = typename fetch::math::Tensor<int>;

TEST(graph_test, node_placeholder)
{
  fetch::ml::Graph<ArrayType> g;
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});

  std::shared_ptr<ArrayType> data = std::make_shared<ArrayType>(8);
  std::shared_ptr<ArrayType> gt   = std::make_shared<ArrayType>(8);
  int                        i(0);
  for (int e : {1, 2, 3, 4, 5, 6, 7, 8})
  {
    data->Set(std::uint64_t(i), e);
    gt->Set(std::uint64_t(i), e);
    i++;
  }

  g.SetInput("Input", data);
  std::shared_ptr<ArrayType> prediction = g.Evaluate("Input");

  // test correct values
  ASSERT_TRUE(prediction->AllClose(*gt));
}

TEST(graph_test, node_relu)
{
  fetch::ml::Graph<ArrayType> g;
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});
  g.AddNode<fetch::ml::ops::Relu<ArrayType>>("Relu", {"Input"});

  std::shared_ptr<ArrayType> data =
      std::make_shared<ArrayType>(std::vector<typename ArrayType::SizeType>({4, 4}));
  std::shared_ptr<ArrayType> gt =
      std::make_shared<ArrayType>(std::vector<typename ArrayType::SizeType>({4, 4}));
  std::vector<int> dataValues({0, -1, 2, -3, 4, -5, 6, -7, 8, -9, 10, -11, 12, -13, 14, -15, 16});
  std::vector<int> gtValues({0, 0, 2, 0, 4, 0, 6, 0, 8, 0, 10, 0, 12, 0, 14, 0, 16});
  for (int i(0); i < 4; ++i)
  {
    for (int j(0); j < 4; ++j)
    {
      data->Set(std::vector<std::uint64_t>({std::uint64_t(i), std::uint64_t(j)}),
                dataValues[std::uint64_t(i * 4 + j)]);
      gt->Set(std::vector<std::uint64_t>({std::uint64_t(i), std::uint64_t(j)}),
              gtValues[std::uint64_t(i * 4 + j)]);
    }
  }

  g.SetInput("Input", data);
  std::shared_ptr<fetch::math::Tensor<int>> prediction = g.Evaluate("Relu");

  // test correct values
  ASSERT_TRUE(prediction->AllClose(*gt));
}
