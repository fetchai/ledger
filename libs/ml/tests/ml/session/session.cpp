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

// #include <algorithm>
#include <iomanip>
#include <iostream>
#include <gtest/gtest.h>

#include "math/linalg/matrix.hpp"
#include "ml/ops/ops.hpp"

//#include "math/ndarray.hpp"
//#include "math/free_functions/free_functions.hpp"

using namespace fetch::ml;
#define ARRAY_SIZE 100

using Type = double;
using ArrayType = fetch::math::linalg::Matrix<Type>;
using VariableType = fetch::ml::Variable<ArrayType>;

void AssignVariableIncrement(VariableType &var, Type val = 0.0, Type incr = 1.0)
{
  for (std::size_t i = 0; i < var.shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < var.shape()[1]; ++j)
    {
      var.Set(i, j, val);
      val += incr;
    }
  }
}
void AssignArray(ArrayType &var, Type val = 1.0)
{
  for (std::size_t i = 0; i < var.shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < var.shape()[1]; ++j)
    {
      var.Set(i, j, val);
    }
  }
}
void AssignArray(ArrayType &var, std::vector<Type> vec_val)
{
  std::size_t counter = 0;
  for (std::size_t i = 0; i < var.shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < var.shape()[1]; ++j)
    {
      var.Set(i, j, vec_val[counter]);
      ++counter;
    }
  }
}

TEST(session_test, trivial_backprop_test)
{

  // set up session
  SessionManager<ArrayType, VariableType> sess{};

  // set up some variables
  std::vector<std::size_t> l1_shape{2, 3};
  std::vector<std::size_t> l2_shape{3, 4};
  Variable<ArrayType> l1{sess, l1_shape};
  Variable<ArrayType> l2{sess, l2_shape};
  AssignVariableIncrement(l1, 1.0);
  AssignVariableIncrement(l2, 1.0);

  // Dot product
  Variable<ArrayType> ret = fetch::ml::ops::Dot(l1, l2, sess);

  // test shape
  ASSERT_TRUE(ret.shape()[0] == l1_shape[0]);
  ASSERT_TRUE(ret.shape()[1] == l2_shape[1]);

  // assign ground truth
  std::vector<Type> gt_vec{38, 44, 50, 56, 83, 98, 113, 128};
  ArrayType gt{ret.shape()};
  AssignArray(gt, gt_vec);

  // Forward Prop
  sess.Forward(l1, ret.variable_name());

  // test correct values
  ASSERT_TRUE(ret.data().AllClose(gt));

  // BackProp
  sess.BackwardGraph(ret);

  // generate ground truth for gradients
  ArrayType gt_grad{ret.grad().shape()};
  AssignArray(gt_grad, 1.0);

  // test gradients
  ASSERT_TRUE(ret.grad().AllClose(gt_grad));

  // Assign and Dot a new variable
  std::vector<std::size_t> l3_shape{4, 7};
  Variable<ArrayType> l3{sess, l3_shape};
  AssignVariableIncrement(l3);
  Variable<ArrayType> ret2 = fetch::ml::ops::Dot(ret, l3, sess);

  // BackProp
  sess.BackwardGraph(ret2);

  // generate ground truth for gradients
  ArrayType gt_grad2{ret2.grad().shape()};
  AssignArray(gt_grad2, 1.0);
  ASSERT_TRUE(ret2.grad().AllClose(gt_grad2));

}





TEST(session_test, dot_sigmoid_backprop_test)
{
//
//  // set up session
//  SessionManager<ArrayType, VariableType> sess{};
//
//  std::size_t data_points = 7;
//  std::size_t input_size = 10;
//  std::size_t h1_size = 1;
//  std::size_t output_size = 1;
//
////  Type alpha = 0.1;
////  std::size_t n_reps = 10;
//
//  // set up some variables
//  std::vector<std::size_t> l1_activations_shape{data_points, input_size}; // 7 data points x 10 input size
//  std::vector<std::size_t> l1_weights_shape{input_size, h1_size};    // 10 input size x 15 neurons
//  std::vector<std::size_t> l2_weights_shape{h1_size, output_size};    // 15 neurons x 2 output neurons
//  std::vector<std::size_t> gt_shape{data_points, output_size};    //
//
//  Variable<ArrayType> l1_activations{sess, l1_activations_shape};
//  Variable<ArrayType> l1_weights{sess, l1_weights_shape};
//  Variable<ArrayType> l2_weights{sess, l2_weights_shape};
//  Variable<ArrayType> gt{sess, gt_shape};
//
//  AssignVariableIncrement(l1_activations, 0.1, 0.1);
//  AssignVariableIncrement(l1_weights, -0.1, 0.1);
//  AssignVariableIncrement(l2_weights, -0.1, 0.1);
//  AssignVariableIncrement(gt, 10.0, 0.1);
//
//  // 1 layer
//  Variable<ArrayType> ret = fetch::ml::ops::Dot(l1_activations, l1_weights, sess);
////  Variable<ArrayType> ret2 = fetch::ml::ops::Sigmoid(ret, sess);
//
//  sess.Forward(l1_activations, ret.variable_name());
//
//
////  // 2 layer
////  Variable<ArrayType> ret3 = fetch::ml::ops::Dot(ret2, l2_weights, sess);
////  Variable<ArrayType> ret4 = fetch::ml::ops::Sigmoid(ret3, sess);
//
////  Variable<ArrayType> ret5 = fetch::ml::ops::MeanSquareError(ret2, gt, sess);
////
////  sess.Forward(l1_activations, ret5.variable_name());
////
////  for (std::size_t k = 0; k < n_reps; ++k)
////  {
////
////
////    if ((k == 0) || (k == (n_reps-1)))
////    {
////      std::cout << "k: " << k << std::endl;
////
////      for (std::size_t i = 0; i < l1_weights.grad().shape()[0]; ++i)
////      {
////        for (std::size_t j = 0; j < l1_weights.grad().shape()[1]; ++j)
////        {
////          std::cout << "l1_weights.grad().At(i, j)" << l1_weights.grad().At(i, j) << std::endl;
////        }
////      }
////      for (std::size_t i = 0; i < l1_weights.data().shape()[0]; ++i)
////      {
////        for (std::size_t j = 0; j < l1_weights.data().shape()[1]; ++j)
////        {
////          std::cout << "l1_weights.data().At(i, j)" << l1_weights.data().At(i, j) << std::endl;
////        }
////      }
////
////      for (std::size_t i = 0; i < ret.grad().shape()[0]; ++i)
////      {
////        for (std::size_t j = 0; j < ret.grad().shape()[1]; ++j)
////        {
////          std::cout << "ret.grad().At(i, j)" << ret.grad().At(i, j) << std::endl;
////        }
////      }
////      for (std::size_t i = 0; i < ret.data().shape()[0]; ++i)
////      {
////        for (std::size_t j = 0; j < ret.data().shape()[1]; ++j)
////        {
////          std::cout << "ret.data().At(i, j)" << ret.data().At(i, j) << std::endl;
////        }
////      }
////
////      for (std::size_t i = 0; i < ret2.grad().shape()[0]; ++i)
////      {
////        for (std::size_t j = 0; j < ret2.grad().shape()[1]; ++j)
////        {
////          std::cout << "ret2.grad().At(i, j)" << ret2.grad().At(i, j) << std::endl;
////        }
////      }
////      for (std::size_t i = 0; i < ret2.data().shape()[0]; ++i)
////      {
////        for (std::size_t j = 0; j < ret2.data().shape()[1]; ++j)
////        {
////          std::cout << "ret2.data().At(i, j)" << ret2.data().At(i, j) << std::endl;
////        }
////      }
////
////      for (std::size_t i = 0; i < ret5.grad().shape()[0]; ++i)
////      {
////        for (std::size_t j = 0; j < ret5.grad().shape()[1]; ++j)
////        {
////          std::cout << "ret5.grad().At(i, j)" << ret5.grad().At(i, j) << std::endl;
////        }
////      }
////      for (std::size_t i = 0; i < ret5.data().shape()[0]; ++i)
////      {
////        for (std::size_t j = 0; j < ret5.data().shape()[1]; ++j)
////        {
////          std::cout << "ret5.data().At(i, j)" << ret5.data().At(i, j) << std::endl;
////        }
////      }
////    }
////
////    sess.BackProp(ret5, alpha);
////  }

}