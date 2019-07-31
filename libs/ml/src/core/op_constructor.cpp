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


#include "ml/layers/PRelu.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/convolution_2d.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/self_attention.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/layers/layers_declaration.hpp"

#include "ml/ops/abs.hpp"
#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/activations/elu.hpp"
#include "ml/ops/activations/leaky_relu.hpp"
#include "ml/ops/activations/logsigmoid.hpp"
#include "ml/ops/activations/logsoftmax.hpp"
#include "ml/ops/activations/randomised_relu.hpp"
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


namespace fetch {
namespace ml {
namespace ops {

template <typename T, typename Params ...>
static void OpConstructor<T, Params ...>::operator(OpType const & operation_type, std::shared_ptr<ops::Ops<T>> op_ptr, Params ... params)
{
  switch (operation_type)
  {
    case OpType::PLACEHOLDER:
    {
      op_ptr_ = fetch::ml::ops::PlaceHolder<ArrayType>(params ...);
      break;
    }
    case OpType::WEIGHTS:
    {
      op_ptr_ = fetch::ml::ops::Weights<ArrayType>(params ...);
      break;
    }
    case OpType::DROPOUT:
    {
      op_ptr_ = fetch::ml::ops::Dropout<ArrayType>(params ...);
      break;
    }
    case OpType::LEAKY_RELU:
    {
      op_ptr_ = fetch::ml::ops::LeakyRelu<ArrayType>(params ...);
      break;
    }
    case OpType::RANDOMISED_RELU:
    {
      op_ptr_ = fetch::ml::ops::RandomisedRelu<ArrayType>(params ...);
      break;
    }
    case OpType::SOFTMAX:
    {
      op_ptr_ = fetch::ml::ops::Softmax<ArrayType>(params ...);
      break;
    }
    case OpType::CONVOLUTION_1D:
    {
      op_ptr_ = fetch::ml::ops::Convolution1D<ArrayType>(params ...);
      break;
    }
    case OpType::MAX_POOL_1D:
    {
      op_ptr_ = fetch::ml::ops::MaxPool1D<ArrayType>(params ...);
      break;
    }
    case OpType::MAX_POOL_2D:
    {
      op_ptr_ = fetch::ml::ops::MaxPool2D<ArrayType>(params ...);
      break;
    }
    case OpType::TRANSPOSE:
    {
      op_ptr_ = fetch::ml::ops::Transpose<ArrayType>(params ...);
      break;
    }
    case OpType::RESHAPE:
    {
      op_ptr_ = fetch::ml::ops::Reshape<ArrayType>(params ...);
      break;
    }
    case OpType::LAYER_FULLY_CONNECTED:
    {
      op_ptr_ = fetch::ml::layers::FullyConnected<ArrayType>(params ...);
      break;
    }
    case OpType::LAYER_CONVOLUTION_1D:
    {
      op_ptr_ = fetch::ml::layers::Convolution1D<ArrayType>(params ...);
      break;
    }
    case OpType::LAYER_CONVOLUTION_2D:
    {
      op_ptr_ = fetch::ml::layers::Convolution2D<ArrayType>(params ...);
      break;
    }
    default:
    {
      throw std::runtime_error("Unknown type for Serialization");
    }
  }
}

} // ops

} // ml
} // fetch
