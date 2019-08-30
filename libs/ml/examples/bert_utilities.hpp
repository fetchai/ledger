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

#include "../../math/include/math/tensor.hpp"
#include "../include/ml/core/graph.hpp"
#include "../include/ml/layers/fully_connected.hpp"
#include "../include/ml/layers/normalisation/layer_norm.hpp"
#include "../include/ml/layers/self_attention_encoder.hpp"
#include "../include/ml/ops/add.hpp"
#include "../include/ml/ops/embeddings.hpp"
#include "../include/ml/ops/loss_functions/cross_entropy_loss.hpp"

using DataType   = float;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = typename TensorType::SizeType;
using SizeVector = typename TensorType::SizeVector;
using GraphType  = typename fetch::ml::Graph<TensorType>;

struct BERTConfig
{
  // the default config is for bert base uncased pretrained model
  SizeType n_encoder_layers  = 12u;
  SizeType max_seq_len       = 512u;
  SizeType model_dims        = 768u;
  SizeType n_heads           = 12u;
  SizeType ff_dims           = 3072u;
  SizeType vocab_size        = 30522u;
  SizeType segment_size      = 2u;
  DataType epsilon           = static_cast<DataType>(1e-12);
  DataType dropout_keep_prob = static_cast<DataType>(0.9);
};

struct BERTInterface
{
  // the default names for input and outpus of a Fetch bert model
  std::vector<std::string> inputs = {"Segment", "Position", "Tokens", "Mask"};
  std::vector<std::string> outputs;

  BERTInterface(BERTConfig const &config)
  {
    outputs.emplace_back("norm_embed");
    for (SizeType i = static_cast<SizeType>(0); i < config.n_encoder_layers; i++)
    {
      outputs.emplace_back("SelfAttentionEncoder_No_" + std::to_string(i));
    }
  }
};

std::pair<std::vector<std::string>, std::vector<std::string>> MakeBertModel(
    BERTConfig const &config, GraphType &g)
{
  // create a bert model based on the config passed in
  SizeType n_encoder_layers  = config.n_encoder_layers;
  SizeType max_seq_len       = config.max_seq_len;
  SizeType model_dims        = config.model_dims;
  SizeType n_heads           = config.n_heads;
  SizeType ff_dims           = config.ff_dims;
  SizeType vocab_size        = config.vocab_size;
  SizeType segment_size      = config.segment_size;
  DataType epsilon           = config.epsilon;
  DataType dropout_keep_prob = config.dropout_keep_prob;

  // initiate graph
  std::string segment = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Segment", {});
  std::string position =
      g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Position", {});
  std::string tokens = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Tokens", {});
  std::string mask   = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Mask", {});

  // create embedding layer
  std::string segment_embedding = g.template AddNode<fetch::ml::ops::Embeddings<TensorType>>(
      "Segment_Embedding", {segment}, model_dims, segment_size);
  std::string position_embedding = g.template AddNode<fetch::ml::ops::Embeddings<TensorType>>(
      "Position_Embedding", {position}, model_dims, max_seq_len);
  std::string token_embedding = g.template AddNode<fetch::ml::ops::Embeddings<TensorType>>(
      "Token_Embedding", {tokens}, model_dims, vocab_size);

  // summing these embeddings up
  std::string seg_pos_sum_embed = g.template AddNode<fetch::ml::ops::Add<TensorType>>(
      "seg_pos_add", {segment_embedding, position_embedding});
  std::string sum_embed = g.template AddNode<fetch::ml::ops::Add<TensorType>>(
      "all_input_add", {token_embedding, seg_pos_sum_embed});

  // create layernorm layer
  std::string norm_embed = g.template AddNode<fetch::ml::layers::LayerNorm<TensorType>>(
      "norm_embed", {sum_embed}, SizeVector({model_dims, 1}), 0u, epsilon);

  // add layers as well as weights
  std::string              layer_output = norm_embed;
  std::vector<std::string> encoder_outputs;
  encoder_outputs.emplace_back(layer_output);
  for (SizeType i = 0u; i < n_encoder_layers; i++)
  {
    // create the encoding layer first
    layer_output = g.template AddNode<fetch::ml::layers::SelfAttentionEncoder<TensorType>>(
        "SelfAttentionEncoder_No_" + std::to_string(i), {layer_output, mask}, n_heads, model_dims,
        ff_dims, dropout_keep_prob, dropout_keep_prob, dropout_keep_prob, epsilon);
    // store layer output names
    encoder_outputs.emplace_back(layer_output);
  }

  return std::make_pair(std::vector<std::string>({segment, position, tokens, mask}),
                        encoder_outputs);
}

void EvaluateGraph(GraphType &g, std::vector<std::string> input_nodes, std::string output_node,
                   std::vector<TensorType> input_data, TensorType output_data, bool verbose = true)
{
  // Evaluate the model classification performance on a set of test data.
  std::cout << "Starting forward passing for manual evaluation on: " << output_data.shape(1)
            << std::endl;
  if (verbose)
  {
    std::cout << "correct label | guessed label | sample loss" << std::endl;
  }
  DataType total_val_loss  = 0;
  DataType correct_counter = static_cast<DataType>(0);
  for (SizeType b = 0; b < static_cast<SizeType>(output_data.shape(1)); b++)
  {
    for (SizeType i = 0; i < static_cast<SizeType>(4); i++)
    {
      g.SetInput(input_nodes[i], input_data[i].View(b).Copy());
    }
    TensorType model_output = g.Evaluate(output_node, false);
    DataType   val_loss =
        fetch::math::CrossEntropyLoss<TensorType>(model_output, output_data.View(b).Copy());
    total_val_loss += val_loss;

    // count correct guesses
    if (model_output.At(0, 0) > static_cast<DataType>(0.5) &&
        output_data.At(0, b) == static_cast<DataType>(1))
    {
      correct_counter++;
    }
    else if (model_output.At(0, 0) < static_cast<DataType>(0.5) &&
             output_data.At(0, b) == static_cast<DataType>(0))
    {
      correct_counter++;
    }

    // show guessed values
    if (verbose)
    {
      std::cout << output_data.At(0, b) << " | " << model_output.At(0, 0) << " | " << val_loss
                << std::endl;
    }
  }
  std::cout << "val acc: " << correct_counter / static_cast<DataType>(output_data.shape(1))
            << std::endl;
  std::cout << "total val loss: " << total_val_loss / static_cast<DataType>(output_data.shape(1))
            << std::endl;
}
