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

#include "core/filesystem/read_file_contents.hpp"
#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/normalisation/layer_norm.hpp"
#include "ml/layers/self_attention_encoder.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

using DataType      = float;
using TensorType    = fetch::math::Tensor<DataType>;
using SizeType      = typename TensorType::SizeType;
using SizeVector    = typename TensorType::SizeVector;
using GraphType     = typename fetch::ml::Graph<TensorType>;
using StateDictType = typename fetch::ml::StateDict<TensorType>;

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

  explicit BERTInterface(BERTConfig const &config)
  {
    outputs.emplace_back("norm_embed");
    for (SizeType i = 0; i < config.n_encoder_layers; i++)
    {
      outputs.emplace_back("SelfAttentionEncoder_No_" + std::to_string(i));
    }
  }
};

inline std::pair<std::vector<std::string>, std::vector<std::string>> MakeBertModel(
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

inline void EvaluateGraph(GraphType &g, std::vector<std::string> input_nodes,
                          std::string const &output_node, std::vector<TensorType> input_data,
                          TensorType output_data, bool verbose = true)
{
  // Evaluate the model classification performance on a set of test data.
  std::cout << "Starting forward passing for manual evaluation on: " << output_data.shape(1)
            << std::endl;
  if (verbose)
  {
    std::cout << "correct label | guessed label | sample loss" << std::endl;
  }
  DataType total_val_loss  = 0;
  auto     correct_counter = static_cast<DataType>(0);
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

inline TensorType LoadTensorFromFile(std::string const &file_name)
{
  std::ifstream weight_file(file_name);
  assert(weight_file.is_open());

  std::string weight_str;
  getline(weight_file, weight_str);
  weight_file.close();

  return TensorType::FromString(weight_str);
}

inline void PutWeightInLayerNorm(StateDictType &state_dict, SizeType model_dims,
                                 std::string const &gamma_file_name,
                                 std::string const &beta_file_name,
                                 std::string const &gamma_weight_name,
                                 std::string const &beta_weight_name)
{
  // load embedding layernorm gamma beta weights
  TensorType layernorm_gamma = LoadTensorFromFile(gamma_file_name);
  TensorType layernorm_beta  = LoadTensorFromFile(beta_file_name);
  assert(layernorm_beta.size() == model_dims);
  assert(layernorm_gamma.size() == model_dims);
  layernorm_beta.Reshape({model_dims, 1, 1});
  layernorm_gamma.Reshape({model_dims, 1, 1});

  // load weights to layernorm layer
  *(state_dict.dict_[gamma_weight_name].weights_) = layernorm_gamma;
  *(state_dict.dict_[beta_weight_name].weights_)  = layernorm_beta;
}

inline void PutWeightInFullyConnected(StateDictType &state_dict, SizeType in_size,
                                      SizeType out_size, std::string const &weights_file_name,
                                      std::string const &bias_file_name,
                                      std::string const &weights_name, std::string const &bias_name)
{
  // load embedding layernorm gamma beta weights
  TensorType weights = LoadTensorFromFile(weights_file_name);
  TensorType bias    = LoadTensorFromFile(bias_file_name);
  FETCH_UNUSED(in_size);
  assert(weights.shape() == SizeVector({out_size, in_size}));
  assert(bias.size() == out_size);
  bias.Reshape({out_size, 1, 1});

  // load weights to layernorm layer
  *(state_dict.dict_[weights_name].weights_) = weights;
  *(state_dict.dict_[bias_name].weights_)    = bias;
}

inline void PutWeightInMultiheadAttention(
    StateDictType &state_dict, SizeType n_heads, SizeType model_dims,
    std::string const &query_weights_file_name, std::string const &query_bias_file_name,
    std::string const &key_weights_file_name, std::string const &key_bias_file_name,
    std::string const &value_weights_file_name, std::string const &value_bias_file_name,
    std::string const &query_weights_name, std::string const &query_bias_name,
    std::string const &key_weights_name, std::string const &key_bias_name,
    std::string const &value_weights_name, std::string const &value_bias_name,
    std::string const &mattn_prefix)
{
  // get weight arrays from file
  TensorType query_weights = LoadTensorFromFile(query_weights_file_name);
  TensorType query_bias    = LoadTensorFromFile(query_bias_file_name);
  query_bias.Reshape({model_dims, 1, 1});
  TensorType key_weights = LoadTensorFromFile(key_weights_file_name);
  TensorType key_bias    = LoadTensorFromFile(key_bias_file_name);
  key_bias.Reshape({model_dims, 1, 1});
  TensorType value_weights = LoadTensorFromFile(value_weights_file_name);
  TensorType value_bias    = LoadTensorFromFile(value_bias_file_name);
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
    assert(sliced_value_weights.shape() == SizeVector({attn_head_size, model_dims}));
    assert(sliced_value_bias.shape() == SizeVector({attn_head_size, 1, 1}));
    assert(sliced_query_weights.shape() == SizeVector({attn_head_size, model_dims}));
    assert(sliced_query_bias.shape() == SizeVector({attn_head_size, 1, 1}));
    assert(sliced_key_weights.shape() == SizeVector({attn_head_size, model_dims}));
    assert(sliced_key_bias.shape() == SizeVector({attn_head_size, 1, 1}));

    // put the weights into each head
    *(state_dict.dict_[this_attn_prefix + query_weights_name].weights_) = sliced_query_weights;
    *(state_dict.dict_[this_attn_prefix + query_bias_name].weights_)    = sliced_query_bias;
    *(state_dict.dict_[this_attn_prefix + key_weights_name].weights_)   = sliced_key_weights;
    *(state_dict.dict_[this_attn_prefix + key_bias_name].weights_)      = sliced_key_bias;
    *(state_dict.dict_[this_attn_prefix + value_weights_name].weights_) = sliced_value_weights;
    *(state_dict.dict_[this_attn_prefix + value_bias_name].weights_)    = sliced_value_bias;
  }
}

inline std::pair<std::vector<std::string>, std::vector<std::string>> LoadPretrainedBertModel(
    std::string const &file_path, BERTConfig const &config, GraphType &g)
{
  SizeType n_encoder_layers  = config.n_encoder_layers;
  SizeType max_seq_len       = config.max_seq_len;
  SizeType model_dims        = config.model_dims;
  SizeType n_heads           = config.n_heads;
  SizeType ff_dims           = config.ff_dims;
  SizeType vocab_size        = config.vocab_size;
  SizeType segment_size      = config.segment_size;
  DataType epsilon           = config.epsilon;
  DataType dropout_keep_prob = config.dropout_keep_prob;

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

  // prepare reuseable container
  StateDictType state_dict;

  // load weights to three embeddings
  // ###################################################################################################################

  // load segment embeddings
  TensorType segment_embedding_weights =
      LoadTensorFromFile(file_path + "bert_embeddings_token_type_embeddings_weight");
  segment_embedding_weights = segment_embedding_weights.Transpose();
  assert(segment_embedding_weights.shape() == SizeVector({model_dims, segment_size}));

  // load position embeddings
  TensorType position_embedding_weights =
      LoadTensorFromFile(file_path + "bert_embeddings_position_embeddings_weight");
  position_embedding_weights = position_embedding_weights.Transpose();
  assert(position_embedding_weights.shape() == SizeVector({model_dims, max_seq_len}));

  // load token embeddings
  TensorType token_embedding_weights =
      LoadTensorFromFile(file_path + "bert_embeddings_word_embeddings_weight");
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

  // create layernorm layer and get statedict
  std::string norm_embed = g.template AddNode<fetch::ml::layers::LayerNorm<TensorType>>(
      "norm_embed", {sum_embed}, SizeVector({model_dims, 1}), 0u, epsilon);
  state_dict = std::dynamic_pointer_cast<GraphType>(g.GetNode(norm_embed)->GetOp())->StateDict();

  // load embedding layernorm gamma beta weights
  PutWeightInLayerNorm(state_dict, model_dims, file_path + "bert_embeddings_LayerNorm_gamma",
                       file_path + "bert_embeddings_LayerNorm_beta", "LayerNorm_Gamma",
                       "LayerNorm_Beta");

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
        ff_dims, dropout_keep_prob, dropout_keep_prob, dropout_keep_prob, epsilon);

    // store layer output
    encoder_outputs.emplace_back(layer_output);

    // get state dict for this layer
    state_dict =
        std::dynamic_pointer_cast<GraphType>(g.GetNode(layer_output)->GetOp())->StateDict();

    // get file path prefix
    std::string file_prefix = file_path + "bert_encoder_layer_" + std::to_string(i) + "_";

    // put weights in 2 layer norms
    PutWeightInLayerNorm(state_dict, model_dims, file_prefix + "attention_output_LayerNorm_gamma",
                         file_prefix + "attention_output_LayerNorm_beta",
                         "SelfAttentionEncoder_Attention_Residual_LayerNorm_LayerNorm_Gamma",
                         "SelfAttentionEncoder_Attention_Residual_LayerNorm_LayerNorm_Beta");
    PutWeightInLayerNorm(state_dict, model_dims, file_prefix + "output_LayerNorm_gamma",
                         file_prefix + "output_LayerNorm_beta",
                         "SelfAttentionEncoder_Feedforward_Residual_LayerNorm_LayerNorm_Gamma",
                         "SelfAttentionEncoder_Feedforward_Residual_LayerNorm_LayerNorm_Beta");

    // put weights in ff block and attention linear conversion part
    PutWeightInFullyConnected(
        state_dict, model_dims, ff_dims, file_prefix + "intermediate_dense_weight",
        file_prefix + "intermediate_dense_bias",
        "SelfAttentionEncoder_Feedforward_Feedforward_No_1_TimeDistributed_FullyConnected_Weights",
        "SelfAttentionEncoder_Feedforward_Feedforward_No_1_TimeDistributed_FullyConnected_Bias");
    PutWeightInFullyConnected(
        state_dict, ff_dims, model_dims, file_prefix + "output_dense_weight",
        file_prefix + "output_dense_bias",
        "SelfAttentionEncoder_Feedforward_Feedforward_No_2_TimeDistributed_FullyConnected_Weights",
        "SelfAttentionEncoder_Feedforward_Feedforward_No_2_TimeDistributed_FullyConnected_Bias");
    PutWeightInFullyConnected(state_dict, model_dims, model_dims,
                              file_prefix + "attention_output_dense_weight",
                              file_prefix + "attention_output_dense_bias",
                              "SelfAttentionEncoder_Multihead_Attention_MultiheadAttention_"
                              "Final_Transformation_TimeDistributed_FullyConnected_Weights",
                              "SelfAttentionEncoder_Multihead_Attention_MultiheadAttention_"
                              "Final_Transformation_TimeDistributed_FullyConnected_Bias");

    // put weights to multi head attention
    PutWeightInMultiheadAttention(
        state_dict, n_heads, model_dims, file_prefix + "attention_self_query_weight",
        file_prefix + "attention_self_query_bias", file_prefix + "attention_self_key_weight",
        file_prefix + "attention_self_key_bias", file_prefix + "attention_self_value_weight",
        file_prefix + "attention_self_value_bias",
        "Query_Transform_TimeDistributed_FullyConnected_Weights",
        "Query_Transform_TimeDistributed_FullyConnected_Bias",
        "Key_Transform_TimeDistributed_FullyConnected_Weights",
        "Key_Transform_TimeDistributed_FullyConnected_Bias",
        "Value_Transform_TimeDistributed_FullyConnected_Weights",
        "Value_Transform_TimeDistributed_FullyConnected_Bias",
        "SelfAttentionEncoder_Multihead_Attention_MultiheadAttention_Head_No");
  }

  return std::make_pair(std::vector<std::string>({segment, position, tokens, mask}),
                        encoder_outputs);
}

inline TensorType RunPseudoForwardPass(std::vector<std::string> input_nodes,
                                       std::string output_node, BERTConfig const &config,
                                       GraphType g, SizeType batch_size, bool verbose)
{
  std::string segment      = std::move(input_nodes[0]);
  std::string position     = std::move(input_nodes[1]);
  std::string tokens       = std::move(input_nodes[2]);
  std::string mask         = std::move(input_nodes[3]);
  std::string layer_output = std::move(output_node);

  SizeType max_seq_len = config.max_seq_len;
  SizeType seq_len     = 256u;

  TensorType tokens_data({max_seq_len, batch_size});
  tokens_data.Fill(static_cast<DataType>(1));

  TensorType mask_data({max_seq_len, 1, batch_size});
  for (SizeType i = 0; i < seq_len; i++)
  {
    for (SizeType b = 0; b < batch_size; b++)
    {
      mask_data.Set(i, 0, b, static_cast<DataType>(1));
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
    std::cout << "\nfirst token: \n"
              << output.View(0).Copy().View(0).Copy().ToString() << std::endl;
  }
  return output;
}

inline void SaveGraphToFile(GraphType &g, std::string const &file_name)
{
  // start serializing and writing to file
  fetch::ml::GraphSaveableParams<TensorType> gsp1 = g.GetGraphSaveableParams();
  std::cout << "got saveable params" << std::endl;

  fetch::serializers::LargeObjectSerializeHelper b;
  b << gsp1;
  std::cout << "finish serializing" << std::endl;

  std::ofstream outFile(file_name, std::ios::out | std::ios::binary);
  outFile.write(b.buffer.data().char_pointer(), std::streamsize(b.buffer.size()));
  outFile.close();
  std::cout << b.buffer.size() << std::endl;
  std::cout << "finish writing to file" << std::endl;
}

inline GraphType ReadFileToGraph(std::string const &file_name)
{
  auto cur_time = std::chrono::high_resolution_clock::now();
  // start reading a file and deserializing
  fetch::byte_array::ConstByteArray buffer = fetch::core::ReadContentsOfFile(file_name.c_str());
  std::cout << "The buffer read from file is of size: " << buffer.size() << " bytes" << std::endl;
  fetch::serializers::MsgPackSerializer b(buffer);
  std::cout << "finish loading bytes to serlializer" << std::endl;

  // start deserializing
  b.seek(0);
  fetch::ml::GraphSaveableParams<TensorType> gsp2;
  b >> gsp2;
  std::cout << "finish deserializing" << std::endl;
  auto g = std::make_shared<GraphType>();
  fetch::ml::utilities::BuildGraph<TensorType>(gsp2, g);
  std::cout << "finish rebuilding graph" << std::endl;

  auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(
      std::chrono::high_resolution_clock::now() - cur_time);
  std::cout << "time span: " << static_cast<double>(time_span.count()) << std::endl;

  return *g;
}

inline std::vector<TensorType> PrepareTensorForBert(TensorType const &data,
                                                    BERTConfig const &config)
{
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
      if (data.At(i, b) == static_cast<DataType>(0))
      {
        break;
      }
      mask_data.Set(i, 0, b, static_cast<DataType>(1));
    }
  }
  return {segment_data, position_data, data, mask_data};
}
