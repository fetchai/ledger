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
#include "ml/variable.hpp"

using namespace fetch::ml;
using Type      = double;
using ArrayType = fetch::math::linalg::Matrix<Type>;
using LayerType = fetch::ml::Variable<ArrayType>;

TEST(variable, simple_arithmetic)
{

  // set up sess and variables
  SessionManager<ArrayType, LayerType> sess{};
  std::vector<std::size_t>             l1_shape{2, 4};
  std::vector<std::size_t>             l2_shape{4, 1};
  auto                                 l1 = sess.Variable(l1_shape);
  auto                                 l2 = sess.Variable(l2_shape);

  // fill values into variables
  int counter = -4;
  for (std::size_t i = 0; i < l1_shape[0]; ++i)
  {
    for (std::size_t j = 0; j < l1_shape[1]; ++j)
    {
      l1->Set(i, j, counter);
      counter += 1;
    }
  }
  counter = -2;
  for (std::size_t i = 0; i < l2_shape[0]; ++i)
  {
    for (std::size_t j = 0; j < l2_shape[1]; ++j)
    {
      l2->Set(i, j, counter);
      counter += 1;
    }
  }

  // some neural net like linear algebra
  auto n1 = fetch::ml::ops::Dot(l1, l2, sess);
  auto n2 = fetch::ml::ops::Relu(n1, sess);
  auto n3 = fetch::ml::ops::ReduceSum(n2, 0, sess);

  // backpropagate gradients
  sess.BackProp(l1, n1, 0.1);

  // test gradient values
  counter = -2;
  for (std::size_t i = 0; i < 4; ++i)
  {
    ASSERT_TRUE(l1->grad()[i] == counter);
    ++counter;
  }
  counter = -2;
  for (std::size_t i = 4; i < 8; ++i)
  {
    ASSERT_TRUE(l1->grad()[i] == counter);
    ++counter;
  }
  counter = -4;
  for (std::size_t i = 0; i < 4; ++i)
  {
    ASSERT_TRUE(l2->grad()[i] == counter);
    ++counter;
    ++counter;
  }
  counter = 1;
  for (std::size_t i = 0; i < 2; ++i)
  {
    ASSERT_TRUE(n1->grad()[i] == counter);
  }
  counter = 1;
  for (std::size_t i = 0; i < 2; ++i)
  {
    ASSERT_TRUE(n1->grad()[i] == counter);
  }
}

TEST(variable, trivial_backprop)
{

  SessionManager<ArrayType, LayerType> sess{};

  std::vector<std::size_t> shape1{2, 10};
  std::vector<std::size_t> shape2{10, 2};
  auto                     l1 = sess.Variable(shape1);
  auto                     l2 = sess.Variable(shape2);
  l1->data().FillArange(0, 20);
  l2->data().FillArange(0, 20);

  auto ret = fetch::ml::ops::Dot(l1, l2, sess);

  ASSERT_TRUE(ret->shape()[0] == 2);
  ASSERT_TRUE(ret->shape()[1] == 2);

  sess.BackProp(l1, ret, 0.1);

  ArrayType gt{ret->shape()};
  for (std::size_t i = 0; i < ret->grad().shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < ret->grad().shape()[1]; ++j)
    {
      gt.Set(i, j, 1.0);
    }
  }
  ASSERT_TRUE(ret->grad().AllClose(gt));
}