//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

#include "math/linalg/matrix.hpp"
#include "ml/computation_graph/computation_graph.hpp"
#include "ml/ops/ops.hpp"
#include "ml/session.hpp"
//#include "ml/variable.hpp"
#include "ml/layers/layers.hpp"

using namespace fetch::ml;

TEST(layers, simple_arithmetic)
{
  using ArrayType = fetch::math::linalg::Matrix<double>;
  using LayerType = fetch::ml::layers::Layer<ArrayType>;
  //  using LayerType = fetch::ml::Variable<ArrayType>;

  SessionManager<ArrayType, LayerType> sess{};

  std::vector<std::size_t> l1_shape{2, 4};
  std::vector<std::size_t> l2_shape{4, 1};

  LayerType l1{l1_shape, sess};
  LayerType l2{l2_shape, sess};
  l1.Initialise();
  l2.Initialise();

  int counter = -4;
  for (std::size_t i = 0; i < l1_shape[0]; ++i)
  {
    for (std::size_t j = 0; j < l1_shape[1]; ++j)
    {
      l1.data.Set(i, j, counter);
      counter += 1;
    }
  }

  counter = -2;
  for (std::size_t i = 0; i < l2_shape[0]; ++i)
  {
    for (std::size_t j = 0; j < l2_shape[1]; ++j)
    {
      l2.data.Set(i, j, counter);
      counter += 1;
    }
  }

  LayerType n1 = fetch::ml::ops::Dot(l1, l2, sess);
  LayerType n2 = fetch::ml::ops::Relu(n1, sess);
  LayerType n3 = fetch::ml::ops::Sum(n2, 0, sess);

  sess.BackwardGraph(n1);

  counter = -2;
  for (std::size_t i = 0; i < 4; ++i)
  {
    ASSERT_TRUE(l1.grad[i] == counter);
    ++counter;
  }
  counter = -2;
  for (std::size_t i = 4; i < 8; ++i)
  {
    ASSERT_TRUE(l1.grad[i] == counter);
    ++counter;
  }
  counter = -4;
  for (std::size_t i = 0; i < 4; ++i)
  {
    ASSERT_TRUE(l2.grad[i] == counter);
    ++counter;
    ++counter;
  }
  counter = 1;
  for (std::size_t i = 0; i < 2; ++i)
  {
    ASSERT_TRUE(n1.grad[i] == counter);
  }
  counter = 1;
  for (std::size_t i = 0; i < 2; ++i)
  {
    ASSERT_TRUE(n1.grad[i] == counter);
  }
}
