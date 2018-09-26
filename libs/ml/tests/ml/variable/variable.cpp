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
//#include "math/ndarray.hpp"
#include "ml/computation_graph/computation_graph.hpp"
#include "ml/ops/ops.hpp"
#include "ml/variable.hpp"

using namespace fetch::ml;

TEST(variable, simple_arithmetic)
{
  using type      = double;
  using ArrayType = fetch::math::linalg::Matrix<type>;

  std::vector<std::size_t> l1_shape{2, 4};
  std::vector<std::size_t> l2_shape{4, 1};

  Variable<ArrayType> l1 = Variable<ArrayType>(l1_shape);
  Variable<ArrayType> l2 = Variable<ArrayType>(l2_shape);

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

  Variable<ArrayType> n1 = fetch::ml::ops::Dot(l1, l2);
  Variable<ArrayType> n2 = fetch::ml::ops::Relu(n1);
  Variable<ArrayType> n3 = fetch::ml::ops::Sum(n2);

  computation_graph::BackwardGraph(n3);

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
