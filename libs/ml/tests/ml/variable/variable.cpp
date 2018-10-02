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

TEST(variable, simple_arithmetic)
{
  using ArrayType = fetch::math::linalg::Matrix<double>;
  using LayerType = fetch::ml::Variable<ArrayType>;

  SessionManager<ArrayType, LayerType> sess{};

  std::vector<std::size_t> l1_shape{2, 4};
  std::vector<std::size_t> l2_shape{4, 1};

  Variable<ArrayType> l1{sess, l1_shape};
  Variable<ArrayType> l2{sess, l2_shape};

  int counter = -4;
  for (std::size_t i = 0; i < l1_shape[0]; ++i)
  {
    for (std::size_t j = 0; j < l1_shape[1]; ++j)
    {
      l1.Set(i, j, counter);
      counter += 1;
    }
  }

  counter = -2;
  for (std::size_t i = 0; i < l2_shape[0]; ++i)
  {
    for (std::size_t j = 0; j < l2_shape[1]; ++j)
    {
      l2.Set(i, j, counter);
      counter += 1;
    }
  }

  Variable<ArrayType> n1 = fetch::ml::ops::Dot(l1, l2, sess);
  Variable<ArrayType> n2 = fetch::ml::ops::Relu(n1, sess);
  Variable<ArrayType> n3 = fetch::ml::ops::Sum(n2, 0, sess);

  sess.BackwardGraph(n1);

  counter = -2;
  for (std::size_t i = 0; i < 4; ++i)
  {
    ASSERT_TRUE(l1.grad()[i] == counter);
    ++counter;
  }
  counter = -2;
  for (std::size_t i = 4; i < 8; ++i)
  {
    ASSERT_TRUE(l1.grad()[i] == counter);
    ++counter;
  }
  counter = -4;
  for (std::size_t i = 0; i < 4; ++i)
  {
    ASSERT_TRUE(l2.grad()[i] == counter);
    ++counter;
    ++counter;
  }
  counter = 1;
  for (std::size_t i = 0; i < 2; ++i)
  {
    ASSERT_TRUE(n1.grad()[i] == counter);
  }
  counter = 1;
  for (std::size_t i = 0; i < 2; ++i)
  {
    ASSERT_TRUE(n1.grad()[i] == counter);
  }
}

// TEST(variable, trivial_neural_net)
//{
//  using type      = double;
//  using type = fetch::math::linalg::Matrix<type>;
//
//  std::size_t train_data_size = 10;
//  std::size_t test_data_size  = 10;
//
//  std::size_t in_size   = 1;
//  std::size_t weights_1 = 20;
//  std::size_t weights_2 = 20;
//  std::size_t out_size  = 1;
//
//  std::vector<std::size_t> x_tr_shape{train_data_size, in_size};
//  Variable<type>      x_tr{x_tr_shape};
//  std::vector<std::size_t> y_tr_shape{train_data_size, out_size};
//  Variable<type>      y_tr{y_tr_shape};
//
//  std::vector<std::size_t> x_te_shape{test_data_size, out_size};
//  Variable<type>      x_te{x_te_shape};
//  std::vector<std::size_t> y_te_shape{test_data_size, out_size};
//  Variable<type>      y_te{y_te_shape};
//
//  for (std::size_t k = 0; k < train_data_size; ++k)
//  {
//    x_tr.data[k] = k;
//    y_tr.data[k] = k + 7;
//  }
//
//  for (std::size_t k = 0; k < test_data_size; ++k)
//  {
//    x_te.data[k] = k + 5;
//    y_te.data[k] = k + 5 + 7;
//  }
//
//  std::vector<std::size_t> l1_shape{in_size, weights_1};
//  std::vector<std::size_t> l2_shape{weights_1, weights_2};
//  std::vector<std::size_t> lout_shape{weights_2, out_size};
//
//  Variable<type> layer_1{l1_shape};
//  Variable<type> layer_2{l2_shape};
//  Variable<type> layer_out{lout_shape};
//
//  Variable<type> l1_out = fetch::ml::ops::Dot(x_tr, layer_1);
//  Variable<type> l2_out = fetch::ml::ops::Relu(l1_out);
//  Variable<type> l3_out = fetch::ml::ops::Sum(l2_out, 0);
//
//  std::cout << " x_tr.size(): " << x_tr.data.size() << std::endl;
//  std::cout << " x_tr[i]: " << std::endl;
//  for (std::size_t i = 0; i < x_tr.data.size(); ++i)
//  {
//    std::cout << " " << x_tr.data[i];
//  }
//  std::cout << std::endl;
//
//  std::cout << " l1_out.size(): " << l1_out.data.size() << std::endl;
//  std::cout << " l1_out[i]: " << std::endl;
//  for (std::size_t i = 0; i < l1_out.data.size(); ++i)
//  {
//    std::cout << " " << l1_out.data[i];
//  }
//  std::cout << std::endl;
//
//  std::cout << " l2_out.size(): " << l2_out.data.size() << std::endl;
//  std::cout << " l2_out[i]: " << std::endl;
//  for (std::size_t i = 0; i < l2_out.data.size(); ++i)
//  {
//    std::cout << " " << l2_out.data[i];
//  }
//  std::cout << std::endl;
//
//  std::cout << " l3_out.size(): " << l3_out.data.size() << std::endl;
//  std::cout << " l3_out[i]: " << std::endl;
//  for (std::size_t i = 0; i < l3_out.data.size(); ++i)
//  {
//    std::cout << " " << l3_out.data[i];
//  }
//  std::cout << std::endl;
//
//
////  Variable<type> error = fetch::ml::ops::MeanSquareError(x_tr, layer_1);
//}
