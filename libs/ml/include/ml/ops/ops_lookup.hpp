#pragma once
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
#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/activations/elu.hpp"
#include "ml/ops/activations/leaky_relu.hpp"
#include "ml/ops/activations/logsigmoid.hpp"
#include "ml/ops/activations/logsoftmax.hpp"
#include "ml/ops/activations/randomized_relu.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/concatenate.hpp"
#include "ml/ops/convolution_1d.hpp"
#include "ml/ops/convolution_2d.hpp"
#include "ml/ops/divide.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/exp.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/log.hpp"
#include "ml/ops/loss_functions.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/max_pool_1d.hpp"
#include "ml/ops/max_pool_2d.hpp"
#include "ml/ops/maximum.hpp"
#include "ml/ops/multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/reshape.hpp"
#include "ml/ops/sqrt.hpp"
#include "ml/ops/subtract.hpp"
#include "ml/ops/tanh.hpp"
#include "ml/ops/transpose.hpp"
#include "ml/ops/weights.hpp"

#include <typeindex>

namespace fetch {
namespace ml {
namespace ops {

template <typename GraphType, typename Op>
void MyAddNode(GraphType *g_ptr,  // nb: needs to be raw pointer because it's called with "this"
               std::shared_ptr<SaveableParams> saved_node, std::string node_name,
               std::vector<std::string> const &inputs)
{
  auto castnode = std::dynamic_pointer_cast<typename Op::SPType>(saved_node);
  if (!castnode)
  {
    throw std::runtime_error("Node downcasting failed.");
  }
  g_ptr->template AddNode<Op>(node_name, inputs, *castnode);
}

template <typename ArrayType>
void OpsLookup(
    Graph<ArrayType> *g_ptr,  // nb: needs to be raw pointer because it's called with "this"
    std::shared_ptr<SaveableParams> saved_node, std::string node_name,
    std::vector<std::string> const &inputs)
{
  std::string descrip = saved_node->DESCRIPTOR;

  if (descrip == ops::Dropout<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Dropout<ArrayType>>(g_ptr, saved_node, node_name,
                                                                  inputs);
  }
  else if (descrip == ops::Elu<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Elu<ArrayType>>(g_ptr, saved_node, node_name, inputs);
  }
  else if (descrip == ops::LeakyRelu<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::LeakyRelu<ArrayType>>(g_ptr, saved_node, node_name,
                                                                    inputs);
  }
  else if (descrip == ops::LogSigmoid<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::LogSigmoid<ArrayType>>(g_ptr, saved_node, node_name,
                                                                     inputs);
  }
  else if (descrip == ops::LogSoftmax<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::LogSoftmax<ArrayType>>(g_ptr, saved_node, node_name,
                                                                     inputs);
  }
  else if (descrip == ops::RandomizedRelu<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::RandomizedRelu<ArrayType>>(g_ptr, saved_node,
                                                                         node_name, inputs);
  }
  else if (descrip == ops::Relu<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Relu<ArrayType>>(g_ptr, saved_node, node_name,
                                                               inputs);
  }
  else if (descrip == ops::Sigmoid<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Sigmoid<ArrayType>>(g_ptr, saved_node, node_name,
                                                                  inputs);
  }
  else if (descrip == ops::Softmax<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Softmax<ArrayType>>(g_ptr, saved_node, node_name,
                                                                  inputs);
  }
  else if (descrip == ops::CrossEntropyLoss<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::CrossEntropyLoss<ArrayType>>(g_ptr, saved_node,
                                                                           node_name, inputs);
  }
  else if (descrip == ops::MeanSquareErrorLoss<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::MeanSquareErrorLoss<ArrayType>>(g_ptr, saved_node,
                                                                              node_name, inputs);
  }
  else if (descrip == ops::SoftmaxCrossEntropyLoss<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::SoftmaxCrossEntropyLoss<ArrayType>>(
        g_ptr, saved_node, node_name, inputs);
  }
  else if (descrip == ops::Add<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Add<ArrayType>>(g_ptr, saved_node, node_name, inputs);
  }
  else if (descrip == ops::Concatenate<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Concatenate<ArrayType>>(g_ptr, saved_node, node_name,
                                                                      inputs);
  }
  else if (descrip == ops::Convolution1D<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Convolution1D<ArrayType>>(g_ptr, saved_node,
                                                                        node_name, inputs);
  }
  else if (descrip == ops::Convolution2D<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Convolution2D<ArrayType>>(g_ptr, saved_node,
                                                                        node_name, inputs);
  }
  else if (descrip == ops::Divide<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Divide<ArrayType>>(g_ptr, saved_node, node_name,
                                                                 inputs);
  }
  else if (descrip == ops::Embeddings<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Embeddings<ArrayType>>(g_ptr, saved_node, node_name,
                                                                     inputs);
  }
  else if (descrip == ops::Exp<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Exp<ArrayType>>(g_ptr, saved_node, node_name, inputs);
  }
  else if (descrip == ops::Flatten<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Flatten<ArrayType>>(g_ptr, saved_node, node_name,
                                                                  inputs);
  }
  else if (descrip == ops::LeakyRelu<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::LeakyRelu<ArrayType>>(g_ptr, saved_node, node_name,
                                                                    inputs);
  }
  else if (descrip == ops::Log<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Log<ArrayType>>(g_ptr, saved_node, node_name, inputs);
  }
  else if (descrip == ops::MatrixMultiply<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::MatrixMultiply<ArrayType>>(g_ptr, saved_node,
                                                                         node_name, inputs);
  }
  else if (descrip == ops::MaxPool1D<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::MaxPool1D<ArrayType>>(g_ptr, saved_node, node_name,
                                                                    inputs);
  }
  else if (descrip == ops::MaxPool2D<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::MaxPool2D<ArrayType>>(g_ptr, saved_node, node_name,
                                                                    inputs);
  }
  else if (descrip == ops::Maximum<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Maximum<ArrayType>>(g_ptr, saved_node, node_name,
                                                                  inputs);
  }
  else if (descrip == ops::Multiply<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Multiply<ArrayType>>(g_ptr, saved_node, node_name,
                                                                   inputs);
  }
  else if (descrip == ops::PlaceHolder<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::PlaceHolder<ArrayType>>(g_ptr, saved_node, node_name,
                                                                      inputs);
  }
  else if (descrip == ops::Reshape<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Reshape<ArrayType>>(g_ptr, saved_node, node_name,
                                                                  inputs);
  }
  else if (descrip == ops::Sqrt<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Sqrt<ArrayType>>(g_ptr, saved_node, node_name,
                                                               inputs);
  }
  else if (descrip == ops::Subtract<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Subtract<ArrayType>>(g_ptr, saved_node, node_name,
                                                                   inputs);
  }
  else if (descrip == ops::TanH<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::TanH<ArrayType>>(g_ptr, saved_node, node_name,
                                                               inputs);
  }
  else if (descrip == ops::Transpose<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Transpose<ArrayType>>(g_ptr, saved_node, node_name,
                                                                    inputs);
  }
  else if (descrip == ops::Weights<ArrayType>::DESCRIPTOR)
  {
    MyAddNode<Graph<ArrayType>, typename ops::Weights<ArrayType>>(g_ptr, saved_node, node_name,
                                                                  inputs);
  }
  else
  {
    throw std::runtime_error("Unknown Op type");
  }
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
