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
#include "ml/utilities/graph_builder.hpp"

// include every layer we might need to lookup
#include "ml/layers/PRelu.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/convolution_2d.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/self_attention.hpp"
#include "ml/layers/skip_gram.hpp"

#include "ml/saveparams/saveable_params.hpp"

#include <typeindex>

namespace fetch {
namespace ml {
namespace ops {

template <typename ArrayType>
void OpsLookup(std::shared_ptr<Graph<ArrayType>>               g_ptr,
               std::shared_ptr<SaveableParamsInterface> const &saved_node,
               std::string const &node_name, std::vector<std::string> const &inputs)
{
  std::string descrip = saved_node->DESCRIPTOR;

  if (descrip == ops::Dropout<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Dropout<ArrayType>>(g_ptr, saved_node, node_name,
                                                                inputs);
  }
  else if (descrip == ops::Elu<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Elu<ArrayType>>(g_ptr, saved_node, node_name, inputs);
  }
  else if (descrip == ops::LeakyRelu<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::LeakyRelu<ArrayType>>(g_ptr, saved_node, node_name,
                                                                  inputs);
  }
  else if (descrip == ops::LogSigmoid<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::LogSigmoid<ArrayType>>(g_ptr, saved_node, node_name,
                                                                   inputs);
  }
  else if (descrip == ops::LogSoftmax<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::LogSoftmax<ArrayType>>(g_ptr, saved_node, node_name,
                                                                   inputs);
  }
  else if (descrip == ops::RandomisedRelu<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::RandomisedRelu<ArrayType>>(g_ptr, saved_node, node_name,
                                                                       inputs);
  }
  else if (descrip == ops::Relu<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Relu<ArrayType>>(g_ptr, saved_node, node_name, inputs);
  }
  else if (descrip == ops::Sigmoid<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Sigmoid<ArrayType>>(g_ptr, saved_node, node_name,
                                                                inputs);
  }
  else if (descrip == ops::Softmax<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Softmax<ArrayType>>(g_ptr, saved_node, node_name,
                                                                inputs);
  }
  else if (descrip == ops::CrossEntropyLoss<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::CrossEntropyLoss<ArrayType>>(g_ptr, saved_node,
                                                                         node_name, inputs);
  }
  else if (descrip == ops::MeanSquareErrorLoss<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::MeanSquareErrorLoss<ArrayType>>(g_ptr, saved_node,
                                                                            node_name, inputs);
  }
  else if (descrip == ops::SoftmaxCrossEntropyLoss<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::SoftmaxCrossEntropyLoss<ArrayType>>(g_ptr, saved_node,
                                                                                node_name, inputs);
  }
  else if (descrip == ops::Add<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Add<ArrayType>>(g_ptr, saved_node, node_name, inputs);
  }
  else if (descrip == ops::Concatenate<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Concatenate<ArrayType>>(g_ptr, saved_node, node_name,
                                                                    inputs);
  }
  else if (descrip == ops::Convolution1D<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Convolution1D<ArrayType>>(g_ptr, saved_node, node_name,
                                                                      inputs);
  }
  else if (descrip == ops::Convolution2D<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Convolution2D<ArrayType>>(g_ptr, saved_node, node_name,
                                                                      inputs);
  }
  else if (descrip == ops::Divide<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Divide<ArrayType>>(g_ptr, saved_node, node_name,
                                                               inputs);
  }
  else if (descrip == ops::Embeddings<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Embeddings<ArrayType>>(g_ptr, saved_node, node_name,
                                                                   inputs);
  }
  else if (descrip == ops::Exp<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Exp<ArrayType>>(g_ptr, saved_node, node_name, inputs);
  }
  else if (descrip == ops::Flatten<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Flatten<ArrayType>>(g_ptr, saved_node, node_name,
                                                                inputs);
  }
  else if (descrip == ops::LeakyRelu<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::LeakyRelu<ArrayType>>(g_ptr, saved_node, node_name,
                                                                  inputs);
  }
  else if (descrip == ops::Log<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Log<ArrayType>>(g_ptr, saved_node, node_name, inputs);
  }
  else if (descrip == ops::MatrixMultiply<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::MatrixMultiply<ArrayType>>(g_ptr, saved_node, node_name,
                                                                       inputs);
  }
  else if (descrip == ops::MaxPool1D<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::MaxPool1D<ArrayType>>(g_ptr, saved_node, node_name,
                                                                  inputs);
  }
  else if (descrip == ops::MaxPool2D<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::MaxPool2D<ArrayType>>(g_ptr, saved_node, node_name,
                                                                  inputs);
  }
  else if (descrip == ops::Maximum<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Maximum<ArrayType>>(g_ptr, saved_node, node_name,
                                                                inputs);
  }
  else if (descrip == ops::Multiply<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Multiply<ArrayType>>(g_ptr, saved_node, node_name,
                                                                 inputs);
  }
  else if (descrip == ops::PlaceHolder<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::PlaceHolder<ArrayType>>(g_ptr, saved_node, node_name,
                                                                    inputs);
  }
  else if (descrip == ops::Reshape<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Reshape<ArrayType>>(g_ptr, saved_node, node_name,
                                                                inputs);
  }
  else if (descrip == ops::Sqrt<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Sqrt<ArrayType>>(g_ptr, saved_node, node_name, inputs);
  }
  else if (descrip == ops::Subtract<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Subtract<ArrayType>>(g_ptr, saved_node, node_name,
                                                                 inputs);
  }
  else if (descrip == ops::TanH<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::TanH<ArrayType>>(g_ptr, saved_node, node_name, inputs);
  }
  else if (descrip == ops::Transpose<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Transpose<ArrayType>>(g_ptr, saved_node, node_name,
                                                                  inputs);
  }
  else if (descrip == ops::Weights<ArrayType>::DESCRIPTOR)
  {
    AddNode<Graph<ArrayType>, typename ops::Weights<ArrayType>>(g_ptr, saved_node, node_name,
                                                                inputs);
  }
  //  else if (descrip == layers::FullyConnected<ArrayType>::DESCRIPTOR)
  //  {
  //    MyAddSubgraph<Graph<ArrayType>, typename layers::FullyConnected<ArrayType>>(g_ptr,
  //    saved_node,
  //                                                                            node_name, inputs);
  //
  //    AddNode<Graph<ArrayType>, typename layers::FullyConnected<ArrayType>>(g_ptr, saved_node,
  //                                                                            node_name, inputs);
  //  }
  //  else if (descrip == layers::Convolution1D<ArrayType>::DESCRIPTOR)
  //  {
  //    AddNode<Graph<ArrayType>, typename layers::Convolution1D<ArrayType>>(g_ptr, saved_node,
  //                                                                            node_name, inputs);
  //  }
  //  else if (descrip == layers::Convolution2D<ArrayType>::DESCRIPTOR)
  //  {
  //    AddNode<Graph<ArrayType>, typename layers::Convolution2D<ArrayType>>(g_ptr, saved_node,
  //                                                                            node_name, inputs);
  //  }
  //  else if (descrip == layers::PRelu<ArrayType>::DESCRIPTOR)
  //  {
  //    AddNode<Graph<ArrayType>, typename layers::PRelu<ArrayType>>(g_ptr, saved_node,
  //                                                                            node_name, inputs);
  //  }
  //  else if (descrip == layers::SelfAttention<ArrayType>::DESCRIPTOR)
  //  {
  //    AddNode<Graph<ArrayType>, typename layers::SelfAttention<ArrayType>>(g_ptr, saved_node,
  //                                                                            node_name, inputs);
  //  }
  //  else if (descrip == layers::SkipGram<ArrayType>::DESCRIPTOR)
  //  {
  //    AddNode<Graph<ArrayType>, typename layers::SkipGram<ArrayType>>(g_ptr, saved_node,
  //                                                                            node_name, inputs);
  //  }
  else
  {
    throw std::runtime_error("Unknown Op type " + descrip);
  }
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
