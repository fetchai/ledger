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

#include "core/filesystem/read_file_contents.hpp"
#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "math/metrics/cross_entropy.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/normalisation/layer_norm.hpp"
#include "ml/layers/self_attention_encoder.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/bert_utilities.hpp"
#include "ml/utilities/graph_builder.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace utilities {

template <class TensorType>
TensorType LoadTensorFromFile(std::string const &file_name)
{
  std::ifstream weight_file(file_name);
  assert(weight_file.is_open());

  std::string weight_str;
  getline(weight_file, weight_str);
  weight_file.close();

  return TensorType::FromString(weight_str);
}

template <class TensorType>
void PutWeightInLayerNorm(fetch::ml::Graph<TensorType> &g, SizeType model_dims,
                          std::string const &gamma_file_name, std::string const &beta_file_name,
                          std::string const &gamma_weight_name, std::string const &beta_weight_name)
{
  // load embedding layernorm gamma beta weights
  TensorType layernorm_gamma = LoadTensorFromFile<TensorType>(gamma_file_name);
  TensorType layernorm_beta  = LoadTensorFromFile<TensorType>(beta_file_name);
  assert(layernorm_beta.size() == model_dims);
  assert(layernorm_gamma.size() == model_dims);
  layernorm_beta.Reshape({model_dims, 1, 1});
  layernorm_gamma.Reshape({model_dims, 1, 1});

  // load weights to layernorm layer
  g.SetWeight(gamma_weight_name, layernorm_gamma);
  g.SetWeight(beta_weight_name, layernorm_beta);
}

template <class TensorType>
void PutWeightInFullyConnected(fetch::ml::Graph<TensorType> &g, SizeType in_size, SizeType out_size,
                               std::string const &weights_file_name,
                               std::string const &bias_file_name, std::string const &weights_name,
                               std::string const &bias_name)
{
  // load embedding layernorm gamma beta weights

  TensorType weights = LoadTensorFromFile<TensorType>(weights_file_name);
  TensorType bias    = LoadTensorFromFile<TensorType>(bias_file_name);
  FETCH_UNUSED(in_size);
  assert(weights.shape() == typename TensorType::SizeVector({out_size, in_size}));
  assert(bias.size() == out_size);
  bias.Reshape({out_size, 1, 1});

  // load weights to layernorm layer
  g.SetWeight(weights_name, weights);
  g.SetWeight(bias_name, bias);
}

template <class TensorType>
std::pair<std::vector<std::string>, std::vector<std::string>> MakeBertModel(
    BERTConfig<TensorType> const &config, fetch::ml::Graph<TensorType> &g)
{
  using DataType   = typename TensorType::Type;
  using SizeVector = typename TensorType::SizeVector;
  // create a bert model based on the config passed in
  SizeType n_encoder_layers  = config.n_encoder_layers;
  SizeType max_seq_len       = config.max_seq_len;
  SizeType model_dims        = config.model_dims;
  SizeType n_heads           = config.n_heads;
  SizeType ff_dims           = config.ff_dims;
  SizeType vocab_size        = config.vocab_size;
  SizeType segment_size      = config.segment_size;
  DataType epsilon           = config.epsilon;
  DataType dropout_drop_prob = config.dropout_drop_prob;

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
        ff_dims, dropout_drop_prob, dropout_drop_prob, dropout_drop_prob, epsilon);
    // store layer output names
    encoder_outputs.emplace_back(layer_output);
  }

  return std::make_pair(std::vector<std::string>({segment, position, tokens, mask}),
                        encoder_outputs);
}

template <class TensorType>
void EvaluateGraph(fetch::ml::Graph<TensorType> &g, std::vector<std::string> input_nodes,
                   std::string const &output_node, std::vector<TensorType> input_data,
                   TensorType output_data, bool verbose)
{
  using DataType = typename TensorType::Type;
  // Evaluate the model classification performance on a set of test data.
  std::cout << "Starting forward passing for manual evaluation on: " << output_data.shape(1)
            << std::endl;
  if (verbose)
  {
    std::cout << "correct label | guessed label | sample loss" << std::endl;
  }
  // DataType total_val_loss  = 0;
  auto total_val_loss  = DataType{0};
  auto correct_counter = DataType{0};
  for (SizeType b = 0; b < static_cast<SizeType>(output_data.shape(1)); b++)
  {
    for (SizeType i = 0; i < static_cast<SizeType>(4); i++)
    {
      g.SetInput(input_nodes[i], input_data[i].View(b).Copy());
    }
    TensorType model_output = g.Evaluate(output_node, false);
    DataType   val_loss =
        fetch::math::CrossEntropyLoss<TensorType>(model_output, output_data.View(b).Copy());
    total_val_loss = static_cast<DataType>(total_val_loss + val_loss);

    // count correct guesses
    if (model_output.At(0, 0) > fetch::math::Type<DataType>("0.5") &&
        output_data.At(0, b) == DataType{1})
    {
      correct_counter++;
    }
    else if (model_output.At(0, 0) < fetch::math::Type<DataType>("0.5") &&
             output_data.At(0, b) == DataType{0})
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

template <class TensorType>
void PutWeightInMultiheadAttention(
    fetch::ml::Graph<TensorType> &g, SizeType n_heads, SizeType model_dims,
    std::string const &query_weights_file_name, std::string const &query_bias_file_name,
    std::string const &key_weights_file_name, std::string const &key_bias_file_name,
    std::string const &value_weights_file_name, std::string const &value_bias_file_name,
    std::string const &query_weights_name, std::string const &query_bias_name,
    std::string const &key_weights_name, std::string const &key_bias_name,
    std::string const &value_weights_name, std::string const &value_bias_name,
    std::string const &mattn_prefix, std::string const &layer)
{
  // get weight arrays from file
  TensorType query_weights = LoadTensorFromFile<TensorType>(query_weights_file_name);
  TensorType query_bias    = LoadTensorFromFile<TensorType>(query_bias_file_name);
  query_bias.Reshape({model_dims, 1, 1});
  TensorType key_weights = LoadTensorFromFile<TensorType>(key_weights_file_name);
  TensorType key_bias    = LoadTensorFromFile<TensorType>(key_bias_file_name);
  key_bias.Reshape({model_dims, 1, 1});
  TensorType value_weights = LoadTensorFromFile<TensorType>(value_weights_file_name);
  TensorType value_bias    = LoadTensorFromFile<TensorType>(value_bias_file_name);
  value_bias.Reshape({model_dims, 1, 1});

  // put weights into each head
  SizeType                      attn_head_size = model_dims / n_heads;
  std::pair<SizeType, SizeType> start_end_slice;
  for (SizeType i = 0u; i < n_heads; i++)
  {
    // generating slice indices
    start_end_slice = std::make_pair(i * attn_head_size, (i + 1) * attn_head_size);

    // fullfill attention prefix
    std::string this_attn_prefix = mattn_prefix + "_" + std::to_string(i) + "_";

    // slice the weights
    TensorType sliced_query_weights = query_weights.Slice(start_end_slice, 0u).Copy();
    TensorType sliced_query_bias    = query_bias.Slice(start_end_slice, 0u).Copy();
    TensorType sliced_key_weights   = key_weights.Slice(start_end_slice, 0u).Copy();
    TensorType sliced_key_bias      = key_bias.Slice(start_end_slice, 0u).Copy();
    TensorType sliced_value_weights = value_weights.Slice(start_end_slice, 0u).Copy();
    TensorType sliced_value_bias    = value_bias.Slice(start_end_slice, 0u).Copy();
    assert(sliced_value_weights.shape() ==
           typename TensorType::SizeVector({attn_head_size, model_dims}));
    assert(sliced_value_bias.shape() == typename TensorType::SizeVector({attn_head_size, 1, 1}));
    assert(sliced_query_weights.shape() ==
           typename TensorType::SizeVector({attn_head_size, model_dims}));
    assert(sliced_query_bias.shape() == typename TensorType::SizeVector({attn_head_size, 1, 1}));
    assert(sliced_key_weights.shape() ==
           typename TensorType::SizeVector({attn_head_size, model_dims}));
    assert(sliced_key_bias.shape() == typename TensorType::SizeVector({attn_head_size, 1, 1}));

    std::string prefix = layer;
    prefix += "/";
    prefix += this_attn_prefix;

    // put the weights into each head
    g.SetWeight(prefix + query_weights_name, sliced_query_weights);
    g.SetWeight(prefix + query_bias_name, sliced_query_bias);
    g.SetWeight(prefix + key_weights_name, sliced_key_weights);
    g.SetWeight(prefix + key_bias_name, sliced_key_bias);
    g.SetWeight(prefix + value_weights_name, sliced_value_weights);
    g.SetWeight(prefix + value_bias_name, sliced_value_bias);
  }
}

template <class TensorType>
std::pair<std::vector<std::string>, std::vector<std::string>> LoadPretrainedBertModel(
    std::string const &file_path, BERTConfig<TensorType> const &config,
    fetch::ml::Graph<TensorType> &g)
{
  using DataType             = typename TensorType::Type;
  using SizeVector           = typename TensorType::SizeVector;
  SizeType n_encoder_layers  = config.n_encoder_layers;
  SizeType max_seq_len       = config.max_seq_len;
  SizeType model_dims        = config.model_dims;
  SizeType n_heads           = config.n_heads;
  SizeType ff_dims           = config.ff_dims;
  SizeType vocab_size        = config.vocab_size;
  SizeType segment_size      = config.segment_size;
  DataType epsilon           = config.epsilon;
  DataType dropout_drop_prob = config.dropout_drop_prob;

  // for release version
  FETCH_UNUSED(vocab_size);
  FETCH_UNUSED(max_seq_len);
  FETCH_UNUSED(segment_size);

  // initiate graph
  std::string segment = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Segment", {});
  std::string position =
      g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Position", {});
  std::string tokens = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Tokens", {});
  std::string mask   = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Mask", {});

  // load weights to three embeddings
  // ###################################################################################################################

  // load segment embeddings
  TensorType segment_embedding_weights =
      LoadTensorFromFile<TensorType>(file_path + "bert_embeddings_token_type_embeddings_weight");
  segment_embedding_weights = segment_embedding_weights.Transpose();
  assert(segment_embedding_weights.shape() == SizeVector({model_dims, segment_size}));

  // load position embeddings
  TensorType position_embedding_weights =
      LoadTensorFromFile<TensorType>(file_path + "bert_embeddings_position_embeddings_weight");
  position_embedding_weights = position_embedding_weights.Transpose();
  assert(position_embedding_weights.shape() == SizeVector({model_dims, max_seq_len}));

  // load token embeddings
  TensorType token_embedding_weights =
      LoadTensorFromFile<TensorType>(file_path + "bert_embeddings_word_embeddings_weight");
  token_embedding_weights = token_embedding_weights.Transpose();
  assert(token_embedding_weights.shape() == SizeVector({model_dims, vocab_size}));

  // use these weights to init embedding layers
  std::string segment_embedding = g.template AddNode<fetch::ml::ops::Embeddings<TensorType>>(
      "Segment_Embedding", {segment}, segment_embedding_weights);
  std::string position_embedding = g.template AddNode<fetch::ml::ops::Embeddings<TensorType>>(
      "Position_Embedding", {position}, position_embedding_weights);
  std::string token_embedding = g.template AddNode<fetch::ml::ops::Embeddings<TensorType>>(
      "Token_Embedding", {tokens}, token_embedding_weights);

  // summing these embeddings up
  std::string seg_pos_sum_embed = g.template AddNode<fetch::ml::ops::Add<TensorType>>(
      "seg_pos_add", {segment_embedding, position_embedding});
  std::string sum_embed = g.template AddNode<fetch::ml::ops::Add<TensorType>>(
      "all_input_add", {token_embedding, seg_pos_sum_embed});

  // load weights to layernorm after embedding
  // ###############################################################################################################

  // create layernorm layer
  std::string norm_embed = g.template AddNode<fetch::ml::layers::LayerNorm<TensorType>>(
      "norm_embed", {sum_embed}, SizeVector({model_dims, 1}), 0u, epsilon);

  // load embedding layernorm gamma beta weights
  PutWeightInLayerNorm(g, model_dims, file_path + "bert_embeddings_LayerNorm_gamma",
                       file_path + "bert_embeddings_LayerNorm_beta",
                       norm_embed + "/LayerNorm_Gamma", norm_embed + "/LayerNorm_Beta");

  // load weights to self attention encoding layers
  // ###############################################################################################################

  // add layers as well as weights
  std::string              layer_output = norm_embed;
  std::vector<std::string> encoder_outputs;
  encoder_outputs.emplace_back(layer_output);
  FETCH_UNUSED(n_encoder_layers);
  for (SizeType i = 0u; i < n_encoder_layers; i++)
  {
    // create the encoding layer first
    layer_output = g.template AddNode<fetch::ml::layers::SelfAttentionEncoder<TensorType>>(
        "SelfAttentionEncoder_No_" + std::to_string(i), {layer_output, mask}, n_heads, model_dims,
        ff_dims, dropout_drop_prob, dropout_drop_prob, dropout_drop_prob, epsilon);

    // store layer output
    encoder_outputs.emplace_back(layer_output);

    // get file path prefix
    std::string file_prefix = file_path + "bert_encoder_layer_" + std::to_string(i) + "_";

    auto nodes = g.GetNodeNames();

    // put weights in 2 layer norms
    PutWeightInLayerNorm(
        g, model_dims, file_prefix + "attention_output_LayerNorm_gamma",
        file_prefix + "attention_output_LayerNorm_beta",
        layer_output + "/SelfAttentionEncoder_Attention_Residual_LayerNorm/LayerNorm_Gamma",
        layer_output + "/SelfAttentionEncoder_Attention_Residual_LayerNorm/LayerNorm_Beta");
    PutWeightInLayerNorm(
        g, model_dims, file_prefix + "output_LayerNorm_gamma",
        file_prefix + "output_LayerNorm_beta",
        layer_output + "/SelfAttentionEncoder_Feedforward_Residual_LayerNorm/LayerNorm_Gamma",
        layer_output + "/SelfAttentionEncoder_Feedforward_Residual_LayerNorm/LayerNorm_Beta");

    // put weights in ff block and attention linear conversion part
    PutWeightInFullyConnected(g, model_dims, ff_dims, file_prefix + "intermediate_dense_weight",
                              file_prefix + "intermediate_dense_bias",
                              layer_output +
                                  "/SelfAttentionEncoder_Feedforward_Feedforward_No_1/"
                                  "TimeDistributed_FullyConnected_Weights",
                              layer_output +
                                  "/SelfAttentionEncoder_Feedforward_Feedforward_No_1/"
                                  "TimeDistributed_FullyConnected_Bias");
    PutWeightInFullyConnected(g, ff_dims, model_dims, file_prefix + "output_dense_weight",
                              file_prefix + "output_dense_bias",
                              layer_output +
                                  "/SelfAttentionEncoder_Feedforward_Feedforward_No_2/"
                                  "TimeDistributed_FullyConnected_Weights",
                              layer_output +
                                  "/SelfAttentionEncoder_Feedforward_Feedforward_No_2/"
                                  "TimeDistributed_FullyConnected_Bias");

    PutWeightInFullyConnected(g, model_dims, model_dims,
                              file_prefix + "attention_output_dense_weight",
                              file_prefix + "attention_output_dense_bias",
                              layer_output +
                                  "/SelfAttentionEncoder_Multihead_Attention/MultiheadAttention_"
                                  "Final_Transformation/TimeDistributed_FullyConnected_Weights",
                              layer_output +
                                  "/SelfAttentionEncoder_Multihead_Attention/MultiheadAttention_"
                                  "Final_Transformation/TimeDistributed_FullyConnected_Bias");

    // put weights to multi head attention
    PutWeightInMultiheadAttention(
        g, n_heads, model_dims, file_prefix + "attention_self_query_weight",
        file_prefix + "attention_self_query_bias", file_prefix + "attention_self_key_weight",
        file_prefix + "attention_self_key_bias", file_prefix + "attention_self_value_weight",
        file_prefix + "attention_self_value_bias",
        "Query_Transform/TimeDistributed_FullyConnected_Weights",
        "Query_Transform/TimeDistributed_FullyConnected_Bias",
        "Key_Transform/TimeDistributed_FullyConnected_Weights",
        "Key_Transform/TimeDistributed_FullyConnected_Bias",
        "Value_Transform/TimeDistributed_FullyConnected_Weights",
        "Value_Transform/TimeDistributed_FullyConnected_Bias",
        "SelfAttentionEncoder_Multihead_Attention/MultiheadAttention_Head_No", layer_output);
  }

  return std::make_pair(std::vector<std::string>({segment, position, tokens, mask}),
                        encoder_outputs);
}

template <class TensorType>
TensorType RunPseudoForwardPass(std::vector<std::string> input_nodes, std::string output_node,
                                BERTConfig<TensorType> const &config,
                                fetch::ml::Graph<TensorType> g, SizeType batch_size, bool verbose)
{
  using DataType           = typename TensorType::Type;
  std::string segment      = std::move(input_nodes[0]);
  std::string position     = std::move(input_nodes[1]);
  std::string tokens       = std::move(input_nodes[2]);
  std::string mask         = std::move(input_nodes[3]);
  std::string layer_output = std::move(output_node);

  SizeType max_seq_len = config.max_seq_len;
  SizeType seq_len     = 256u;

  TensorType tokens_data({max_seq_len, batch_size});
  tokens_data.Fill(DataType{1});

  TensorType mask_data({max_seq_len, 1, batch_size});
  for (SizeType i = 0; i < seq_len; i++)
  {
    for (SizeType b = 0; b < batch_size; b++)
    {
      mask_data.Set(i, 0, b, DataType{1});
    }
  }
  TensorType position_data({max_seq_len, batch_size});
  for (SizeType i = 0; i < seq_len; i++)
  {
    for (SizeType b = 0; b < batch_size; b++)
    {
      position_data.Set(i, b, static_cast<DataType>(i));
    }
  }
  TensorType segment_data({max_seq_len, batch_size});
  g.SetInput(segment, segment_data);
  g.SetInput(position, position_data);
  g.SetInput(tokens, tokens_data);
  g.SetInput(mask, mask_data);

  std::cout << "Starting forward passing on " << batch_size << " batches." << std::endl;
  auto cur_time  = std::chrono::high_resolution_clock::now();
  auto output    = g.Evaluate(layer_output, false);
  auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(
      std::chrono::high_resolution_clock::now() - cur_time);
  std::cout << "time span: " << static_cast<double>(time_span.count()) << std::endl;
  std::cout << "time span per batch: "
            << static_cast<double>(time_span.count()) / static_cast<double>(batch_size)
            << std::endl;
  if (verbose)
  {
    for (std::size_t i = 0; i < output.shape().size(); i++)
    {
      std::cout << " | " << output.shape(i);
    }
    // show the first token representation of the first batch of the specified output layer's output
    std::cout << "\nfirst token: \n" << output.View(0).Copy().View(0).ToString() << std::endl;
  }
  return output;
}

template <class TensorType>
std::vector<TensorType> PrepareTensorForBert(TensorType const &            data,
                                             BERTConfig<TensorType> const &config)
{
  using DataType       = typename TensorType::Type;
  SizeType max_seq_len = config.max_seq_len;
  // check that data shape is proper for bert input
  if (data.shape().size() != 2 || data.shape(0) != max_seq_len)
  {
    throw fetch::ml::exceptions::InvalidMode("Incorrect data shape for given bert config");
  }

  // build segment, mask and pos data for each sentence in the data
  SizeType batch_size = data.shape(1);

  // segment data and position data need no adjustment, they are universal for all input during
  // finetuning
  TensorType segment_data({max_seq_len, batch_size});
  TensorType position_data({max_seq_len, batch_size});
  for (SizeType i = 0; i < max_seq_len; i++)
  {
    for (SizeType b = 0; b < batch_size; b++)
    {
      position_data.Set(i, b, static_cast<DataType>(i));
    }
  }

  // mask data is the only one that is dependent on token data
  TensorType mask_data({max_seq_len, 1, batch_size});
  for (SizeType b = 0; b < batch_size; b++)
  {
    for (SizeType i = 0; i < max_seq_len; i++)
    {
      // stop filling 1 to mask if current position is 0 for token
      if (data.At(i, b) == DataType{0})
      {
        break;
      }
      mask_data.Set(i, 0, b, DataType{1});
    }
  }
  return {segment_data, position_data, data, mask_data};
}

template math::Tensor<int8_t> LoadTensorFromFile<math::Tensor<int8_t>>(
    std::string const &file_name);
template math::Tensor<int16_t> LoadTensorFromFile<math::Tensor<int16_t>>(
    std::string const &file_name);
template math::Tensor<int32_t> LoadTensorFromFile<math::Tensor<int32_t>>(
    std::string const &file_name);
template math::Tensor<int64_t> LoadTensorFromFile<math::Tensor<int64_t>>(
    std::string const &file_name);
template math::Tensor<float>  LoadTensorFromFile<math::Tensor<float>>(std::string const &file_name);
template math::Tensor<double> LoadTensorFromFile<math::Tensor<double>>(
    std::string const &file_name);
template math::Tensor<fixed_point::fp32_t> LoadTensorFromFile<math::Tensor<fixed_point::fp32_t>>(
    std::string const &file_name);
template math::Tensor<fixed_point::fp64_t> LoadTensorFromFile<math::Tensor<fixed_point::fp64_t>>(
    std::string const &file_name);
template math::Tensor<fixed_point::fp128_t> LoadTensorFromFile<math::Tensor<fixed_point::fp128_t>>(
    std::string const &file_name);

template void PutWeightInLayerNorm<math::Tensor<int8_t>>(fetch::ml::Graph<math::Tensor<int8_t>> &g,
                                                         SizeType           model_dims,
                                                         std::string const &gamma_file_name,
                                                         std::string const &beta_file_name,
                                                         std::string const &gamma_weight_name,
                                                         std::string const &beta_weight_name);
template void PutWeightInLayerNorm<math::Tensor<int16_t>>(
    fetch::ml::Graph<math::Tensor<int16_t>> &g, SizeType model_dims,
    std::string const &gamma_file_name, std::string const &beta_file_name,
    std::string const &gamma_weight_name, std::string const &beta_weight_name);
template void PutWeightInLayerNorm<math::Tensor<int32_t>>(
    fetch::ml::Graph<math::Tensor<int32_t>> &g, SizeType model_dims,
    std::string const &gamma_file_name, std::string const &beta_file_name,
    std::string const &gamma_weight_name, std::string const &beta_weight_name);
template void PutWeightInLayerNorm<math::Tensor<int64_t>>(
    fetch::ml::Graph<math::Tensor<int64_t>> &g, SizeType model_dims,
    std::string const &gamma_file_name, std::string const &beta_file_name,
    std::string const &gamma_weight_name, std::string const &beta_weight_name);
template void PutWeightInLayerNorm<math::Tensor<float>>(fetch::ml::Graph<math::Tensor<float>> &g,
                                                        SizeType           model_dims,
                                                        std::string const &gamma_file_name,
                                                        std::string const &beta_file_name,
                                                        std::string const &gamma_weight_name,
                                                        std::string const &beta_weight_name);
template void PutWeightInLayerNorm<math::Tensor<double>>(fetch::ml::Graph<math::Tensor<double>> &g,
                                                         SizeType           model_dims,
                                                         std::string const &gamma_file_name,
                                                         std::string const &beta_file_name,
                                                         std::string const &gamma_weight_name,
                                                         std::string const &beta_weight_name);
template void PutWeightInLayerNorm<math::Tensor<fixed_point::fp32_t>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp32_t>> &g, SizeType model_dims,
    std::string const &gamma_file_name, std::string const &beta_file_name,
    std::string const &gamma_weight_name, std::string const &beta_weight_name);
template void PutWeightInLayerNorm<math::Tensor<fixed_point::fp64_t>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp64_t>> &g, SizeType model_dims,
    std::string const &gamma_file_name, std::string const &beta_file_name,
    std::string const &gamma_weight_name, std::string const &beta_weight_name);
template void PutWeightInLayerNorm<math::Tensor<fixed_point::fp128_t>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp128_t>> &g, SizeType model_dims,
    std::string const &gamma_file_name, std::string const &beta_file_name,
    std::string const &gamma_weight_name, std::string const &beta_weight_name);

template void PutWeightInFullyConnected<math::Tensor<int8_t>>(
    fetch::ml::Graph<math::Tensor<int8_t>> &g, SizeType in_size, SizeType out_size,
    std::string const &weights_file_name, std::string const &bias_file_name,
    std::string const &weights_name, std::string const &bias_name);
template void PutWeightInFullyConnected<math::Tensor<int16_t>>(
    fetch::ml::Graph<math::Tensor<int16_t>> &g, SizeType in_size, SizeType out_size,
    std::string const &weights_file_name, std::string const &bias_file_name,
    std::string const &weights_name, std::string const &bias_name);
template void PutWeightInFullyConnected<math::Tensor<int32_t>>(
    fetch::ml::Graph<math::Tensor<int32_t>> &g, SizeType in_size, SizeType out_size,
    std::string const &weights_file_name, std::string const &bias_file_name,
    std::string const &weights_name, std::string const &bias_name);
template void PutWeightInFullyConnected<math::Tensor<int64_t>>(
    fetch::ml::Graph<math::Tensor<int64_t>> &g, SizeType in_size, SizeType out_size,
    std::string const &weights_file_name, std::string const &bias_file_name,
    std::string const &weights_name, std::string const &bias_name);
template void PutWeightInFullyConnected<math::Tensor<float>>(
    fetch::ml::Graph<math::Tensor<float>> &g, SizeType in_size, SizeType out_size,
    std::string const &weights_file_name, std::string const &bias_file_name,
    std::string const &weights_name, std::string const &bias_name);
template void PutWeightInFullyConnected<math::Tensor<double>>(
    fetch::ml::Graph<math::Tensor<double>> &g, SizeType in_size, SizeType out_size,
    std::string const &weights_file_name, std::string const &bias_file_name,
    std::string const &weights_name, std::string const &bias_name);
template void PutWeightInFullyConnected<math::Tensor<fixed_point::fp32_t>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp32_t>> &g, SizeType in_size, SizeType out_size,
    std::string const &weights_file_name, std::string const &bias_file_name,
    std::string const &weights_name, std::string const &bias_name);
template void PutWeightInFullyConnected<math::Tensor<fixed_point::fp64_t>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp64_t>> &g, SizeType in_size, SizeType out_size,
    std::string const &weights_file_name, std::string const &bias_file_name,
    std::string const &weights_name, std::string const &bias_name);
template void PutWeightInFullyConnected<math::Tensor<fixed_point::fp128_t>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp128_t>> &g, SizeType in_size, SizeType out_size,
    std::string const &weights_file_name, std::string const &bias_file_name,
    std::string const &weights_name, std::string const &bias_name);

template std::pair<std::vector<std::string>, std::vector<std::string>>
MakeBertModel<math::Tensor<int8_t>>(BERTConfig<math::Tensor<int8_t>> const &,
                                    fetch::ml::Graph<math::Tensor<int8_t>> &);
template std::pair<std::vector<std::string>, std::vector<std::string>>
MakeBertModel<math::Tensor<int16_t>>(BERTConfig<math::Tensor<int16_t>> const &,
                                     fetch::ml::Graph<math::Tensor<int16_t>> &);
template std::pair<std::vector<std::string>, std::vector<std::string>>
MakeBertModel<math::Tensor<int32_t>>(BERTConfig<math::Tensor<int32_t>> const &config,
                                     fetch::ml::Graph<math::Tensor<int32_t>> &g);
template std::pair<std::vector<std::string>, std::vector<std::string>>
MakeBertModel<math::Tensor<int64_t>>(BERTConfig<math::Tensor<int64_t>> const &config,
                                     fetch::ml::Graph<math::Tensor<int64_t>> &g);
template std::pair<std::vector<std::string>, std::vector<std::string>>
MakeBertModel<math::Tensor<float>>(BERTConfig<math::Tensor<float>> const &config,
                                   fetch::ml::Graph<math::Tensor<float>> &g);
template std::pair<std::vector<std::string>, std::vector<std::string>>
                                                                       MakeBertModel<math::Tensor<double>>(BERTConfig<math::Tensor<double>> const &config,
                                    fetch::ml::Graph<math::Tensor<double>> &g);
template std::pair<std::vector<std::string>, std::vector<std::string>> MakeBertModel<
    math::Tensor<fixed_point::fp32_t>>(BERTConfig<math::Tensor<fixed_point::fp32_t>> const &config,
                                       fetch::ml::Graph<math::Tensor<fixed_point::fp32_t>> &g);
template std::pair<std::vector<std::string>, std::vector<std::string>> MakeBertModel<
    math::Tensor<fixed_point::fp64_t>>(BERTConfig<math::Tensor<fixed_point::fp64_t>> const &config,
                                       fetch::ml::Graph<math::Tensor<fixed_point::fp64_t>> &g);
template std::pair<std::vector<std::string>, std::vector<std::string>>
MakeBertModel<math::Tensor<fixed_point::fp128_t>>(
    BERTConfig<math::Tensor<fixed_point::fp128_t>> const &config,
    fetch::ml::Graph<math::Tensor<fixed_point::fp128_t>> &g);

template void EvaluateGraph<math::Tensor<int8_t>>(fetch::ml::Graph<math::Tensor<int8_t>> &g,
                                                  std::vector<std::string>          input_nodes,
                                                  std::string const &               output_node,
                                                  std::vector<math::Tensor<int8_t>> input_data,
                                                  math::Tensor<int8_t> output_data, bool verbose);
template void EvaluateGraph<math::Tensor<int16_t>>(fetch::ml::Graph<math::Tensor<int16_t>> &g,
                                                   std::vector<std::string>           input_nodes,
                                                   std::string const &                output_node,
                                                   std::vector<math::Tensor<int16_t>> input_data,
                                                   math::Tensor<int16_t> output_data, bool verbose);
template void EvaluateGraph<math::Tensor<int32_t>>(fetch::ml::Graph<math::Tensor<int32_t>> &g,
                                                   std::vector<std::string>           input_nodes,
                                                   std::string const &                output_node,
                                                   std::vector<math::Tensor<int32_t>> input_data,
                                                   math::Tensor<int32_t> output_data, bool verbose);
template void EvaluateGraph<math::Tensor<int64_t>>(fetch::ml::Graph<math::Tensor<int64_t>> &g,
                                                   std::vector<std::string>           input_nodes,
                                                   std::string const &                output_node,
                                                   std::vector<math::Tensor<int64_t>> input_data,
                                                   math::Tensor<int64_t> output_data, bool verbose);
template void EvaluateGraph<math::Tensor<float>>(fetch::ml::Graph<math::Tensor<float>> &g,
                                                 std::vector<std::string>               input_nodes,
                                                 std::string const &                    output_node,
                                                 std::vector<math::Tensor<float>>       input_data,
                                                 math::Tensor<float> output_data, bool verbose);
template void EvaluateGraph<math::Tensor<double>>(fetch::ml::Graph<math::Tensor<double>> &g,
                                                  std::vector<std::string>          input_nodes,
                                                  std::string const &               output_node,
                                                  std::vector<math::Tensor<double>> input_data,
                                                  math::Tensor<double> output_data, bool verbose);
template void EvaluateGraph<math::Tensor<fixed_point::fp32_t>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp32_t>> &g, std::vector<std::string> input_nodes,
    std::string const &output_node, std::vector<math::Tensor<fixed_point::fp32_t>> input_data,
    math::Tensor<fixed_point::fp32_t> output_data, bool verbose);
template void EvaluateGraph<math::Tensor<fixed_point::fp64_t>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp64_t>> &g, std::vector<std::string> input_nodes,
    std::string const &output_node, std::vector<math::Tensor<fixed_point::fp64_t>> input_data,
    math::Tensor<fixed_point::fp64_t> output_data, bool verbose);
template void EvaluateGraph<math::Tensor<fixed_point::fp128_t>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp128_t>> &g, std::vector<std::string> input_nodes,
    std::string const &output_node, std::vector<math::Tensor<fixed_point::fp128_t>> input_data,
    math::Tensor<fixed_point::fp128_t> output_data, bool verbose);

template void PutWeightInMultiheadAttention<math::Tensor<int8_t>>(
    fetch::ml::Graph<math::Tensor<int8_t>> &g, SizeType n_heads, SizeType model_dims,
    std::string const &query_weights_file_name, std::string const &query_bias_file_name,
    std::string const &key_weights_file_name, std::string const &key_bias_file_name,
    std::string const &value_weights_file_name, std::string const &value_bias_file_name,
    std::string const &query_weights_name, std::string const &query_bias_name,
    std::string const &key_weights_name, std::string const &key_bias_name,
    std::string const &value_weights_name, std::string const &value_bias_name,
    std::string const &mattn_prefix, std::string const &layer);

template void PutWeightInMultiheadAttention<math::Tensor<int16_t>>(
    fetch::ml::Graph<math::Tensor<int16_t>> &g, SizeType n_heads, SizeType model_dims,
    std::string const &query_weights_file_name, std::string const &query_bias_file_name,
    std::string const &key_weights_file_name, std::string const &key_bias_file_name,
    std::string const &value_weights_file_name, std::string const &value_bias_file_name,
    std::string const &query_weights_name, std::string const &query_bias_name,
    std::string const &key_weights_name, std::string const &key_bias_name,
    std::string const &value_weights_name, std::string const &value_bias_name,
    std::string const &mattn_prefix, std::string const &layer);

template void PutWeightInMultiheadAttention<math::Tensor<int32_t>>(
    fetch::ml::Graph<math::Tensor<int32_t>> &g, SizeType n_heads, SizeType model_dims,
    std::string const &query_weights_file_name, std::string const &query_bias_file_name,
    std::string const &key_weights_file_name, std::string const &key_bias_file_name,
    std::string const &value_weights_file_name, std::string const &value_bias_file_name,
    std::string const &query_weights_name, std::string const &query_bias_name,
    std::string const &key_weights_name, std::string const &key_bias_name,
    std::string const &value_weights_name, std::string const &value_bias_name,
    std::string const &mattn_prefix, std::string const &layer);

template void PutWeightInMultiheadAttention<math::Tensor<int64_t>>(
    fetch::ml::Graph<math::Tensor<int64_t>> &g, SizeType n_heads, SizeType model_dims,
    std::string const &query_weights_file_name, std::string const &query_bias_file_name,
    std::string const &key_weights_file_name, std::string const &key_bias_file_name,
    std::string const &value_weights_file_name, std::string const &value_bias_file_name,
    std::string const &query_weights_name, std::string const &query_bias_name,
    std::string const &key_weights_name, std::string const &key_bias_name,
    std::string const &value_weights_name, std::string const &value_bias_name,
    std::string const &mattn_prefix, std::string const &layer);

template void PutWeightInMultiheadAttention<math::Tensor<float>>(
    fetch::ml::Graph<math::Tensor<float>> &g, SizeType n_heads, SizeType model_dims,
    std::string const &query_weights_file_name, std::string const &query_bias_file_name,
    std::string const &key_weights_file_name, std::string const &key_bias_file_name,
    std::string const &value_weights_file_name, std::string const &value_bias_file_name,
    std::string const &query_weights_name, std::string const &query_bias_name,
    std::string const &key_weights_name, std::string const &key_bias_name,
    std::string const &value_weights_name, std::string const &value_bias_name,
    std::string const &mattn_prefix, std::string const &layer);

template void PutWeightInMultiheadAttention<math::Tensor<double>>(
    fetch::ml::Graph<math::Tensor<double>> &g, SizeType n_heads, SizeType model_dims,
    std::string const &query_weights_file_name, std::string const &query_bias_file_name,
    std::string const &key_weights_file_name, std::string const &key_bias_file_name,
    std::string const &value_weights_file_name, std::string const &value_bias_file_name,
    std::string const &query_weights_name, std::string const &query_bias_name,
    std::string const &key_weights_name, std::string const &key_bias_name,
    std::string const &value_weights_name, std::string const &value_bias_name,
    std::string const &mattn_prefix, std::string const &layer);

template void PutWeightInMultiheadAttention<math::Tensor<fixed_point::fp32_t>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp32_t>> &g, SizeType n_heads, SizeType model_dims,
    std::string const &query_weights_file_name, std::string const &query_bias_file_name,
    std::string const &key_weights_file_name, std::string const &key_bias_file_name,
    std::string const &value_weights_file_name, std::string const &value_bias_file_name,
    std::string const &query_weights_name, std::string const &query_bias_name,
    std::string const &key_weights_name, std::string const &key_bias_name,
    std::string const &value_weights_name, std::string const &value_bias_name,
    std::string const &mattn_prefix, std::string const &layer);

template void PutWeightInMultiheadAttention<math::Tensor<fixed_point::fp64_t>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp64_t>> &g, SizeType n_heads, SizeType model_dims,
    std::string const &query_weights_file_name, std::string const &query_bias_file_name,
    std::string const &key_weights_file_name, std::string const &key_bias_file_name,
    std::string const &value_weights_file_name, std::string const &value_bias_file_name,
    std::string const &query_weights_name, std::string const &query_bias_name,
    std::string const &key_weights_name, std::string const &key_bias_name,
    std::string const &value_weights_name, std::string const &value_bias_name,
    std::string const &mattn_prefix, std::string const &layer);

template void PutWeightInMultiheadAttention<math::Tensor<fixed_point::fp128_t>>(
    fetch::ml::Graph<math::Tensor<fixed_point::fp128_t>> &g, SizeType n_heads, SizeType model_dims,
    std::string const &query_weights_file_name, std::string const &query_bias_file_name,
    std::string const &key_weights_file_name, std::string const &key_bias_file_name,
    std::string const &value_weights_file_name, std::string const &value_bias_file_name,
    std::string const &query_weights_name, std::string const &query_bias_name,
    std::string const &key_weights_name, std::string const &key_bias_name,
    std::string const &value_weights_name, std::string const &value_bias_name,
    std::string const &mattn_prefix, std::string const &layer);

template std::pair<std::vector<std::string>, std::vector<std::string>>
LoadPretrainedBertModel<math::Tensor<int8_t>>(std::string const &                     file_path,
                                              BERTConfig<math::Tensor<int8_t>> const &config,
                                              fetch::ml::Graph<math::Tensor<int8_t>> &g);

template std::pair<std::vector<std::string>, std::vector<std::string>>
LoadPretrainedBertModel<math::Tensor<int16_t>>(std::string const &                      file_path,
                                               BERTConfig<math::Tensor<int16_t>> const &config,
                                               fetch::ml::Graph<math::Tensor<int16_t>> &g);

template std::pair<std::vector<std::string>, std::vector<std::string>>
LoadPretrainedBertModel<math::Tensor<int32_t>>(std::string const &                      file_path,
                                               BERTConfig<math::Tensor<int32_t>> const &config,
                                               fetch::ml::Graph<math::Tensor<int32_t>> &g);

template std::pair<std::vector<std::string>, std::vector<std::string>>
LoadPretrainedBertModel<math::Tensor<int64_t>>(std::string const &                      file_path,
                                               BERTConfig<math::Tensor<int64_t>> const &config,
                                               fetch::ml::Graph<math::Tensor<int64_t>> &g);

template std::pair<std::vector<std::string>, std::vector<std::string>>
LoadPretrainedBertModel<math::Tensor<float>>(std::string const &                    file_path,
                                             BERTConfig<math::Tensor<float>> const &config,
                                             fetch::ml::Graph<math::Tensor<float>> &g);

template std::pair<std::vector<std::string>, std::vector<std::string>>
LoadPretrainedBertModel<math::Tensor<double>>(std::string const &                     file_path,
                                              BERTConfig<math::Tensor<double>> const &config,
                                              fetch::ml::Graph<math::Tensor<double>> &g);

template std::pair<std::vector<std::string>, std::vector<std::string>>
LoadPretrainedBertModel<math::Tensor<fixed_point::fp32_t>>(
    std::string const &file_path, BERTConfig<math::Tensor<fixed_point::fp32_t>> const &config,
    fetch::ml::Graph<math::Tensor<fixed_point::fp32_t>> &g);

template std::pair<std::vector<std::string>, std::vector<std::string>>
LoadPretrainedBertModel<math::Tensor<fixed_point::fp64_t>>(
    std::string const &file_path, BERTConfig<math::Tensor<fixed_point::fp64_t>> const &config,
    fetch::ml::Graph<math::Tensor<fixed_point::fp64_t>> &g);

template std::pair<std::vector<std::string>, std::vector<std::string>>
LoadPretrainedBertModel<math::Tensor<fixed_point::fp128_t>>(
    std::string const &file_path, BERTConfig<math::Tensor<fixed_point::fp128_t>> const &config,
    fetch::ml::Graph<math::Tensor<fixed_point::fp128_t>> &g);

template math::Tensor<int8_t> RunPseudoForwardPass<math::Tensor<int8_t>>(
    std::vector<std::string> input_nodes, std::string output_node,
    BERTConfig<math::Tensor<int8_t>> const &config, fetch::ml::Graph<math::Tensor<int8_t>> g,
    SizeType batch_size, bool verbose);

template math::Tensor<int16_t> RunPseudoForwardPass<math::Tensor<int16_t>>(
    std::vector<std::string> input_nodes, std::string output_node,
    BERTConfig<math::Tensor<int16_t>> const &config, fetch::ml::Graph<math::Tensor<int16_t>> g,
    SizeType batch_size, bool verbose);

template math::Tensor<int32_t> RunPseudoForwardPass<math::Tensor<int32_t>>(
    std::vector<std::string> input_nodes, std::string output_node,
    BERTConfig<math::Tensor<int32_t>> const &config, fetch::ml::Graph<math::Tensor<int32_t>> g,
    SizeType batch_size, bool verbose);

template math::Tensor<int64_t> RunPseudoForwardPass<math::Tensor<int64_t>>(
    std::vector<std::string> input_nodes, std::string output_node,
    BERTConfig<math::Tensor<int64_t>> const &config, fetch::ml::Graph<math::Tensor<int64_t>> g,
    SizeType batch_size, bool verbose);

template math::Tensor<float> RunPseudoForwardPass<math::Tensor<float>>(
    std::vector<std::string> input_nodes, std::string output_node,
    BERTConfig<math::Tensor<float>> const &config, fetch::ml::Graph<math::Tensor<float>> g,
    SizeType batch_size, bool verbose);

template math::Tensor<double> RunPseudoForwardPass<math::Tensor<double>>(
    std::vector<std::string> input_nodes, std::string output_node,
    BERTConfig<math::Tensor<double>> const &config, fetch::ml::Graph<math::Tensor<double>> g,
    SizeType batch_size, bool verbose);

template math::Tensor<fixed_point::fp32_t> RunPseudoForwardPass<math::Tensor<fixed_point::fp32_t>>(
    std::vector<std::string> input_nodes, std::string output_node,
    BERTConfig<math::Tensor<fixed_point::fp32_t>> const &config,
    fetch::ml::Graph<math::Tensor<fixed_point::fp32_t>> g, SizeType batch_size, bool verbose);

template math::Tensor<fixed_point::fp64_t> RunPseudoForwardPass<math::Tensor<fixed_point::fp64_t>>(
    std::vector<std::string> input_nodes, std::string output_node,
    BERTConfig<math::Tensor<fixed_point::fp64_t>> const &config,
    fetch::ml::Graph<math::Tensor<fixed_point::fp64_t>> g, SizeType batch_size, bool verbose);

template math::Tensor<fixed_point::fp128_t>
RunPseudoForwardPass<math::Tensor<fixed_point::fp128_t>>(
    std::vector<std::string> input_nodes, std::string output_node,
    BERTConfig<math::Tensor<fixed_point::fp128_t>> const &config,
    fetch::ml::Graph<math::Tensor<fixed_point::fp128_t>> g, SizeType batch_size, bool verbose);

template std::vector<math::Tensor<int8_t>> PrepareTensorForBert(
    math::Tensor<int8_t> const &data, BERTConfig<math::Tensor<int8_t>> const &config);

template std::vector<math::Tensor<int16_t>> PrepareTensorForBert(
    math::Tensor<int16_t> const &data, BERTConfig<math::Tensor<int16_t>> const &config);

template std::vector<math::Tensor<int32_t>> PrepareTensorForBert(
    math::Tensor<int32_t> const &data, BERTConfig<math::Tensor<int32_t>> const &config);

template std::vector<math::Tensor<int64_t>> PrepareTensorForBert(
    math::Tensor<int64_t> const &data, BERTConfig<math::Tensor<int64_t>> const &config);

template std::vector<math::Tensor<float>> PrepareTensorForBert(
    math::Tensor<float> const &data, BERTConfig<math::Tensor<float>> const &config);

template std::vector<math::Tensor<double>> PrepareTensorForBert(
    math::Tensor<double> const &data, BERTConfig<math::Tensor<double>> const &config);

template std::vector<math::Tensor<fixed_point::fp32_t>> PrepareTensorForBert(
    math::Tensor<fixed_point::fp32_t> const &            data,
    BERTConfig<math::Tensor<fixed_point::fp32_t>> const &config);

template std::vector<math::Tensor<fixed_point::fp64_t>> PrepareTensorForBert(
    math::Tensor<fixed_point::fp64_t> const &            data,
    BERTConfig<math::Tensor<fixed_point::fp64_t>> const &config);

template std::vector<math::Tensor<fixed_point::fp128_t>> PrepareTensorForBert(
    math::Tensor<fixed_point::fp128_t> const &            data,
    BERTConfig<math::Tensor<fixed_point::fp128_t>> const &config);

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
