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

#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/normalisation/layer_norm.hpp"
#include "ml/layers/self_attention_encoder.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/slice.hpp"
#include "ml/optimisation/adam_optimiser.hpp"

#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType   = float;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = typename TensorType::SizeType;
using SizeVector = typename TensorType::SizeVector;

using GraphType     = typename fetch::ml::Graph<TensorType>;
using StateDictType = typename fetch::ml::StateDict<TensorType>;
using OptimiserType = typename fetch::ml::optimisers::AdamOptimiser<TensorType>;

using RegType         = fetch::ml::RegularisationType;
using WeightsInitType = fetch::ml::ops::WeightsInitialisation;
using ActivationType  = fetch::ml::details::ActivationType;

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

// data creation
TensorType create_position_data(SizeType max_seq_len, SizeType batch_size);
TensorType create_mask_data(SizeType max_seq_len, TensorType seq_len_per_batch);
std::pair<std::vector<TensorType>, TensorType> prepare_data_for_simple_cls(SizeType max_seq_len,
                                                                           SizeType batch_size);
// load weights functionalities
TensorType get_weight_from_file(std::string file_name);
void       put_weight_in_layernorm(StateDictType &state_dict, std::string gamma_file_name,
                                   std::string beta_file_name, std::string gamma_weight_name,
                                   std::string beta_weight_name);
void put_weight_in_fully_connected(StateDictType &state_dict, SizeType in_size, SizeType out_size,
                                   std::string weights_file_name, std::string bias_file_name,
                                   std::string weights_name, std::string bias_name);
void put_weight_in_attention_heads(StateDictType &state_dict, SizeType n_heads, SizeType model_dims,
                                   std::string query_weights_file_name,
                                   std::string query_bias_file_name,
                                   std::string key_weights_file_name,
                                   std::string key_bias_file_name,
                                   std::string value_weights_file_name,
                                   std::string value_bias_file_name, std::string query_weights_name,
                                   std::string query_bias_name, std::string key_weights_name,
                                   std::string key_bias_name, std::string value_weights_name,
                                   std::string value_bias_name, std::string mattn_prefix);
// bert model creation functions
std::pair<std::vector<std::string>, std::string> load_pretrained_bert_model(
    std::string const &file_path, BERTConfig const &config, GraphType &g);
std::pair<std::vector<std::string>, std::string> make_bert_model(BERTConfig const &config,
                                                                 GraphType &       g);

int main(/*int ac, char **av*/)
{
  //
  //  if (ac < 3)
  //  {
  //    std::cout << "Usage : " << av[0]
  //              << " PATH/TO/train-images-idx3-ubyte PATH/TO/train-labels-idx1-ubyte" <<
  //              std::endl;
  //    return 1;
  //  }

  std::cout << "FETCH BERT Demo" << std::endl;

  SizeType    max_seq_len = 512u;
  std::string file_path =
      "/home/xiaodong/Projects/Fetch "
      "scripts/bert_conversion/bert-base-uncased/bert-base-uncased-txt/";
  BERTConfig config;

  GraphType g;
  auto      ret = load_pretrained_bert_model(file_path, config, g);
	
	fetch::ml::GraphSaveableParams<TensorType> gsp1 = g.GetGraphSaveableParams();
	std::cout << "got saveable params" << std::endl;
	fetch::serializers::MsgPackSerializer     b;
	b << gsp1;
	std::cout << "finish serializing" << std::endl;
	b.seek(0);
	auto gsp2 = std::make_shared<fetch::ml::GraphSaveableParams<TensorType>>();
	b >> *gsp2;
	std::cout << "finish deserializing" << std::endl;
	auto g2 = std::make_shared<GraphType>();
	fetch::ml::utilities::BuildGraph<TensorType>(*gsp2, g2);
	std::cout << "recreating graph" << std::endl;

  std::string segment      = ret.first[0];
  std::string position     = ret.first[1];
  std::string tokens       = ret.first[2];
  std::string mask         = ret.first[3];
  std::string layer_output = ret.second;

  //  // Add linear classification layer
  //  std::string cls_token_output = g.template AddNode<fetch::ml::ops::Slice<TensorType>>(
  //      "ClsTokenOutput", {layer_output}, 0u, 1u);
  //  std::string classification_output =
  //      g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
  //          "ClassificationOutput", {cls_token_output}, model_dims, 1u, ActivationType::SIGMOID,
  //          RegType::NONE, static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT, false);
  //  // Set up error signal
  //  std::string label = g.template AddNode<PlaceHolder<TensorType>>("Label", {});
  //  std::string error =
  //      g.template AddNode<CrossEntropyLoss<TensorType>>("Error", {classification_output, label});

  SizeType batch_size = 1u;
  SizeType seq_len    = 256u;

  TensorType tokens_data({max_seq_len, batch_size});
  tokens_data.Fill(static_cast<DataType>(1));

  TensorType mask_data({max_seq_len, max_seq_len, batch_size});
  for (SizeType i = 0; i < seq_len; i++)
  {
    for (SizeType t = 0; t < seq_len; t++)
    {
      for (SizeType b = 0; b < batch_size; b++)
      {
        mask_data.Set(i, t, b, static_cast<DataType>(1));
      }
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

  std::cout << "Starting forward passing" << std::endl;
  auto cur_time  = std::chrono::high_resolution_clock::now();
  auto output    = g.Evaluate(layer_output, false);
  auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(
      std::chrono::high_resolution_clock::now() - cur_time);
  std::cout << "time span: " << static_cast<double>(time_span.count()) << std::endl;
  std::cout << "first token: \n" << output.View(0).Copy().View(0).Copy().ToString() << std::endl;

  //  auto train_data = prepare_data_for_simple_cls(max_seq_len, 30u);
  //  for (SizeType i = 0; i < 100; i++)
  //  {
  //    DataType loss = optimiser.Run(train_data.first, train_data.second);
  //    std::cout << "loss: " << loss << std::endl;
  //  }
  //
  //  std::cout << "Starting forward passing for manual evaluation" << std::endl;
  //  TensorType segment_data  = train_data.first[0];
  //  TensorType position_data = train_data.first[1];
  //  TensorType tokens_data   = train_data.first[2];
  //  TensorType mask_data     = train_data.first[3];
  //  g.SetInput(segment, segment_data);
  //  g.SetInput(position, position_data);
  //  g.SetInput(tokens, tokens_data);
  //  g.SetInput(mask, mask_data);
  //  auto output = g.Evaluate(classification_output, false);
  //  std::cout << "model output: " << output.ToString() << std::endl;
  //  std::cout << "label output: " << train_data.second.ToString() << std::endl;

  return 0;
}

TensorType get_weight_from_file(std::string file_name)
{
  std::ifstream weight_file(file_name);
  assert(weight_file.is_open());

  std::string weight_str;
  getline(weight_file, weight_str);
  weight_file.close();

  return TensorType::FromString(weight_str);
}

void put_weight_in_layernorm(StateDictType &state_dict, SizeType model_dims,
                             std::string gamma_file_name, std::string beta_file_name,
                             std::string gamma_weight_name, std::string beta_weight_name)
{
  // load embedding layernorm gamma beta weights
  TensorType layernorm_gamma = get_weight_from_file(gamma_file_name);
  TensorType layernorm_beta  = get_weight_from_file(beta_file_name);
  assert(layernorm_beta.size() == model_dims);
  assert(layernorm_gamma.size() == model_dims);
  layernorm_beta.Reshape({model_dims, 1, 1});
  layernorm_gamma.Reshape({model_dims, 1, 1});

  // load weights to layernorm layer
  *(state_dict.dict_[gamma_weight_name].weights_) = layernorm_gamma;
  *(state_dict.dict_[beta_weight_name].weights_)  = layernorm_beta;
}

void put_weight_in_fully_connected(StateDictType &state_dict, SizeType in_size, SizeType out_size,
                                   std::string weights_file_name, std::string bias_file_name,
                                   std::string weights_name, std::string bias_name)
{
  // load embedding layernorm gamma beta weights
  TensorType weights = get_weight_from_file(weights_file_name);
  TensorType bias    = get_weight_from_file(bias_file_name);
  FETCH_UNUSED(in_size);
  assert(weights.shape() == SizeVector({out_size, in_size}));
  assert(bias.size() == out_size);
  bias.Reshape({out_size, 1, 1});

  // load weights to layernorm layer
  *(state_dict.dict_[weights_name].weights_) = weights;
  *(state_dict.dict_[bias_name].weights_)    = bias;
}

void put_weight_in_attention_heads(StateDictType &state_dict, SizeType n_heads, SizeType model_dims,
                                   std::string query_weights_file_name,
                                   std::string query_bias_file_name,
                                   std::string key_weights_file_name,
                                   std::string key_bias_file_name,
                                   std::string value_weights_file_name,
                                   std::string value_bias_file_name, std::string query_weights_name,
                                   std::string query_bias_name, std::string key_weights_name,
                                   std::string key_bias_name, std::string value_weights_name,
                                   std::string value_bias_name, std::string mattn_prefix)
{
  // get weight arrays from file
  TensorType query_weights = get_weight_from_file(query_weights_file_name);
  TensorType query_bias    = get_weight_from_file(query_bias_file_name);
  query_bias.Reshape({model_dims, 1, 1});
  TensorType key_weights = get_weight_from_file(key_weights_file_name);
  TensorType key_bias    = get_weight_from_file(key_bias_file_name);
  key_bias.Reshape({model_dims, 1, 1});
  TensorType value_weights = get_weight_from_file(value_weights_file_name);
  TensorType value_bias    = get_weight_from_file(value_bias_file_name);
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

std::pair<std::vector<std::string>, std::string> make_bert_model(BERTConfig const &config,
                                                                 GraphType &       g)
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

  // create layernorm layer and get statedict
  std::string norm_embed = g.template AddNode<fetch::ml::layers::LayerNorm<TensorType>>(
      "norm_embed", {sum_embed}, SizeVector({model_dims, 1}), 0u, epsilon);

  // add layers as well as weights
  std::string layer_output = norm_embed;
  for (SizeType i = 0u; i < n_encoder_layers; i++)
  {
    // create the encoding layer first
    layer_output = g.template AddNode<fetch::ml::layers::SelfAttentionEncoder<TensorType>>(
        "SelfAttentionEncoder_No_" + std::to_string(i), {layer_output, mask}, n_heads, model_dims,
        ff_dims, dropout_keep_prob, epsilon);
  }

  return std::make_pair(std::vector<std::string>({segment, position, tokens, mask}), layer_output);
}

std::pair<std::vector<std::string>, std::string> load_pretrained_bert_model(
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
      get_weight_from_file(file_path + "bert_embeddings_token_type_embeddings_weight");
  segment_embedding_weights = segment_embedding_weights.Transpose();
  assert(segment_embedding_weights.shape() == SizeVector({model_dims, segment_size}));

  // load position embeddings
  TensorType position_embedding_weights =
      get_weight_from_file(file_path + "bert_embeddings_position_embeddings_weight");
  position_embedding_weights = position_embedding_weights.Transpose();
  assert(position_embedding_weights.shape() == SizeVector({model_dims, max_seq_len}));

  // load token embeddings
  TensorType token_embedding_weights =
      get_weight_from_file(file_path + "bert_embeddings_word_embeddings_weight");
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
  put_weight_in_layernorm(state_dict, model_dims, file_path + "bert_embeddings_LayerNorm_gamma",
                          file_path + "bert_embeddings_LayerNorm_beta", "LayerNorm_Gamma",
                          "LayerNorm_Beta");

  // load weights to self attention encoding layers
  // ###############################################################################################################

  // add layers as well as weights
  std::string layer_output = norm_embed;
  FETCH_UNUSED(n_encoder_layers);
  for (SizeType i = 0u; i < n_encoder_layers; i++)
  {
    // create the encoding layer first
    layer_output = g.template AddNode<fetch::ml::layers::SelfAttentionEncoder<TensorType>>(
        "SelfAttentionEncoder_No_" + std::to_string(i), {layer_output, mask}, n_heads, model_dims,
        ff_dims, dropout_keep_prob, epsilon);

    // get state dict for this layer
    state_dict =
        std::dynamic_pointer_cast<GraphType>(g.GetNode(layer_output)->GetOp())->StateDict();

    // get file path prefix
    std::string file_prefix = file_path + "bert_encoder_layer_" + std::to_string(i) + "_";

    // put weights in 2 layer norms
    put_weight_in_layernorm(state_dict, model_dims,
                            file_prefix + "attention_output_LayerNorm_gamma",
                            file_prefix + "attention_output_LayerNorm_beta",
                            "SelfAttentionEncoder_Attention_Residual_LayerNorm_LayerNorm_Gamma",
                            "SelfAttentionEncoder_Attention_Residual_LayerNorm_LayerNorm_Beta");
    put_weight_in_layernorm(state_dict, model_dims, file_prefix + "output_LayerNorm_gamma",
                            file_prefix + "output_LayerNorm_beta",
                            "SelfAttentionEncoder_Feedforward_Residual_LayerNorm_LayerNorm_Gamma",
                            "SelfAttentionEncoder_Feedforward_Residual_LayerNorm_LayerNorm_Beta");

    // put weights in ff block and attention linear conversion part
    put_weight_in_fully_connected(
        state_dict, model_dims, ff_dims, file_prefix + "intermediate_dense_weight",
        file_prefix + "intermediate_dense_bias",
        "SelfAttentionEncoder_Feedforward_Feedforward_No_1_TimeDistributed_FullyConnected_Weights",
        "SelfAttentionEncoder_Feedforward_Feedforward_No_1_TimeDistributed_FullyConnected_Bias");
    put_weight_in_fully_connected(
        state_dict, ff_dims, model_dims, file_prefix + "output_dense_weight",
        file_prefix + "output_dense_bias",
        "SelfAttentionEncoder_Feedforward_Feedforward_No_2_TimeDistributed_FullyConnected_Weights",
        "SelfAttentionEncoder_Feedforward_Feedforward_No_2_TimeDistributed_FullyConnected_Bias");
    put_weight_in_fully_connected(state_dict, model_dims, model_dims,
                                  file_prefix + "attention_output_dense_weight",
                                  file_prefix + "attention_output_dense_bias",
                                  "SelfAttentionEncoder_Multihead_Attention_MultiheadAttention_"
                                  "Final_Transformation_TimeDistributed_FullyConnected_Weights",
                                  "SelfAttentionEncoder_Multihead_Attention_MultiheadAttention_"
                                  "Final_Transformation_TimeDistributed_FullyConnected_Bias");

    // put weights to multi head attention
    put_weight_in_attention_heads(
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

  return std::make_pair(std::vector<std::string>({segment, position, tokens, mask}), layer_output);
}

std::pair<std::vector<TensorType>, TensorType> prepare_data_for_simple_cls(SizeType max_seq_len,
                                                                           SizeType batch_size)
{
  TensorType segment_data({max_seq_len, batch_size});
  TensorType position_data = create_position_data(max_seq_len, batch_size);
  TensorType token_data({max_seq_len, batch_size});
  TensorType mask_data({max_seq_len, max_seq_len, batch_size});
  TensorType labels({1u, batch_size});
  mask_data.Fill(static_cast<DataType>(1));

  for (SizeType i = 0; i < batch_size; i++)
  {
    if (i % 4 == 0)
    {  // all 1
      for (SizeType entry = 1; entry < max_seq_len; entry++)
      {
        token_data.Set(entry, i, static_cast<DataType>(1));
      }
      labels.Set(0u, i, static_cast<DataType>(0));
    }
    else if (i % 4 == 2)
    {  // all 2
      for (SizeType entry = 1; entry < max_seq_len; entry++)
      {
        token_data.Set(entry, i, static_cast<DataType>(2));
      }
      labels.Set(0u, i, static_cast<DataType>(0));
    }
    else
    {  // interval data
      for (SizeType entry = 1; entry < max_seq_len; entry++)
      {
        if (entry % 2 == 1)
        {
          token_data.Set(entry, i, static_cast<DataType>(1));
        }
        else
        {
          token_data.Set(entry, i, static_cast<DataType>(2));
        }
      }
      labels.Set(0u, i, static_cast<DataType>(1));
    }
  }

  return std::make_pair(
      std::vector<TensorType>({segment_data, position_data, token_data, mask_data}), labels);
}

TensorType create_position_data(SizeType max_seq_len, SizeType batch_size)
{
  TensorType ret_position({max_seq_len, batch_size});
  for (SizeType i = 0; i < max_seq_len; i++)
  {
    for (SizeType b = 0; b < batch_size; b++)
    {
      ret_position.Set(i, b, static_cast<DataType>(i));
    }
  }
  return ret_position;
}

TensorType create_mask_data(SizeType max_seq_len, TensorType seq_len_per_batch)
{
  assert(seq_len_per_batch.shape().size() == 2);
  assert(fetch::math::Max(seq_len_per_batch) <= static_cast<DataType>(max_seq_len));
  SizeType   batch_size = seq_len_per_batch.shape(0);
  TensorType ret_mask({max_seq_len, max_seq_len, batch_size});
  for (SizeType b = 0; b < batch_size; b++)
  {
    auto seq_len = static_cast<SizeType>(seq_len_per_batch.At(0, b));
    for (SizeType i = 0; i < seq_len; i++)
    {
      for (SizeType t = 0; t < seq_len; t++)
      {
        ret_mask.Set(i, t, b, static_cast<DataType>(1));
      }
    }
  }
  return ret_mask;
}
