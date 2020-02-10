#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "ml/core/graph.hpp"
#include "ml/utilities/graph_builder.hpp"

// ops includes
#include "ml/ops/abs.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/avg_pool_1d.hpp"
#include "ml/ops/avg_pool_2d.hpp"
#include "ml/ops/concatenate.hpp"
#include "ml/ops/constant.hpp"
#include "ml/ops/convolution_1d.hpp"
#include "ml/ops/convolution_2d.hpp"
#include "ml/ops/divide.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/exp.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/layer_norm.hpp"
#include "ml/ops/log.hpp"
#include "ml/ops/mask_fill.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/max_pool_1d.hpp"
#include "ml/ops/max_pool_2d.hpp"
#include "ml/ops/maximum.hpp"
#include "ml/ops/multiply.hpp"
#include "ml/ops/one_hot.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/prelu_op.hpp"
#include "ml/ops/reduce_mean.hpp"
#include "ml/ops/reshape.hpp"
#include "ml/ops/slice.hpp"
#include "ml/ops/sqrt.hpp"
#include "ml/ops/squeeze.hpp"
#include "ml/ops/strided_slice.hpp"
#include "ml/ops/subtract.hpp"
#include "ml/ops/switch.hpp"
#include "ml/ops/tanh.hpp"
#include "ml/ops/top_k.hpp"
#include "ml/ops/transpose.hpp"
#include "ml/ops/weights.hpp"

// activations includes
#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/activations/elu.hpp"
#include "ml/ops/activations/gelu.hpp"
#include "ml/ops/activations/leaky_relu.hpp"
#include "ml/ops/activations/logsigmoid.hpp"
#include "ml/ops/activations/logsoftmax.hpp"
#include "ml/ops/activations/randomised_relu.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/activations/softmax.hpp"

// loss includes
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/loss_functions/softmax_cross_entropy_loss.hpp"

// metrics includes
#include "ml/ops/metrics/categorical_accuracy.hpp"

// layers includes
#include "ml/layers/PRelu.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/convolution_2d.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/multihead_attention.hpp"
#include "ml/layers/normalisation/layer_norm.hpp"
#include "ml/layers/scaled_dot_product_attention.hpp"
#include "ml/layers/self_attention_encoder.hpp"
#include "ml/layers/skip_gram.hpp"
