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

#include <iomanip>
#include <iostream>

#include "math/linalg/matrix.hpp"
#include "ml/layers/layers.hpp"
#include "ml/ops/ops.hpp"
#include "ml/session.hpp"
//#include "ml/variable.hpp"

using namespace fetch::ml;

using Type            = double;
using ArrayType       = fetch::math::linalg::Matrix<Type>;
using VariableType    = fetch::ml::Variable<ArrayType>;
using VariablePtrType = std::shared_ptr<VariableType>;
using LayerType       = fetch::ml::layers::Layer<ArrayType>;
using LayerPtrType    = std::shared_ptr<LayerType>;

void set_random_data(VariablePtrType x)
{
  for (std::size_t i = 0; i < x->size(); ++i)
  {
    x->data()[i] = 1;
  }
}

void benchmark_large_matrices(std::vector<std::size_t> layer_sizes, bool threading)
{
  // set up session
  SessionManager<ArrayType, VariableType> sess{threading};
  Type                                    alpha = 0.2;
  std::size_t n_reps = 100;  // can be considered value of n_epochs * n_batches

  // set up some variables
  std::size_t data_points = 32;   // single batch size
  std::size_t input_size  = 784;  // # mnist size pixels
  std::size_t h1_size     = layer_sizes[0];
  std::size_t h2_size     = layer_sizes[1];
  std::size_t h3_size     = layer_sizes[2];
  std::size_t output_size = 100;

  std::vector<std::size_t> input_shape{data_points, input_size};
  std::vector<std::size_t> gt_shape{data_points, output_size};

  auto input_data = sess.Variable(input_shape, "Input_data");
  auto l1         = sess.Layer(input_size, h1_size, "LeakyRelu", "layer_1");
  auto l2         = sess.Layer(h1_size, h2_size, "LeakyRelu", "layer_2");
  auto l3         = sess.Layer(h2_size, h3_size, "LeakyRelu", "layer_3");
  auto y_pred     = sess.Layer(h3_size, output_size, "LeakyRelu", "output_layer");
  auto gt         = sess.Variable(gt_shape, "GroundTruth");

  sess.SetInput(l1, input_data);
  sess.SetInput(l2, l1->output());
  sess.SetInput(l3, l2->output());
  sess.SetInput(y_pred, l3->output());

  set_random_data(input_data);
  set_random_data(gt);

  // loss
  auto loss = fetch::ml::ops::MeanSquareError(y_pred->output(), gt, sess);

  // backward pass to get gradient
  sess.BackProp(input_data, loss, alpha, n_reps);

  // forward pass on the computational graph
  auto prediction = sess.Predict(input_data, y_pred->output());
}

int main()
{
  std::cout << "Testing without threading: " << std::endl;
  bool threading = false;
  // TINY NET
  std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
  std::cout << "\t tiny net: " << std::endl;
  benchmark_large_matrices({10, 10, 10}, threading);

  // medium net
  std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
  std::cout << "\t medium net: " << std::endl;
  benchmark_large_matrices({50, 30, 20}, threading);

  // large net
  std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
  std::cout << "\t large net: " << std::endl;
  benchmark_large_matrices({256, 128, 64}, threading);
  std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time_span1 =
      std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
  std::chrono::duration<double> time_span2 =
      std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
  std::chrono::duration<double> time_span3 =
      std::chrono::duration_cast<std::chrono::duration<double>>(t3 - t2);
  std::cout << "mini_net_training_time: " << time_span1.count() << std::endl;
  std::cout << "medium_net_training_time: " << time_span2.count() << std::endl;
  std::cout << "large_net_training_time: " << time_span3.count() << std::endl;

  threading = true;
  // TINY NET
  t0 = std::chrono::high_resolution_clock::now();
  std::cout << "\t tiny net: " << std::endl;
  benchmark_large_matrices({10, 10, 10}, threading);

  // medium net
  t1 = std::chrono::high_resolution_clock::now();
  std::cout << "\t medium net: " << std::endl;
  benchmark_large_matrices({50, 30, 20}, threading);

  // large net
  t2 = std::chrono::high_resolution_clock::now();
  std::cout << "\t large net: " << std::endl;
  benchmark_large_matrices({256, 128, 64}, threading);
  t3 = std::chrono::high_resolution_clock::now();

  time_span1 = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0);
  time_span2 = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
  time_span3 = std::chrono::duration_cast<std::chrono::duration<double>>(t3 - t2);
  std::cout << "mini_net_training_time: " << time_span1.count() << std::endl;
  std::cout << "medium_net_training_time: " << time_span2.count() << std::endl;
  std::cout << "large_net_training_time: " << time_span3.count() << std::endl;

  return 0;
}
