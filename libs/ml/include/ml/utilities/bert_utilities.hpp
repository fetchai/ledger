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

namespace fetch {
namespace ml {
namespace utilities {

using SizeType = fetch::math::SizeType;

template <class TensorType>
struct BERTConfig
{
  using DataType = typename TensorType::Type;
  // the default config is for bert base uncased pretrained model
  SizeType n_encoder_layers  = 12u;
  SizeType max_seq_len       = 512u;
  SizeType model_dims        = 768u;
  SizeType n_heads           = 12u;
  SizeType ff_dims           = 3072u;
  SizeType vocab_size        = 30522u;
  SizeType segment_size      = 2u;
  DataType epsilon           = fetch::math::Type<DataType>("0.000000000001");
  DataType dropout_keep_prob = fetch::math::Type<DataType>("0.9");
};

template <class TensorType>
struct BERTInterface
{
  // the default names for input and outputs of a Fetch bert model
  std::vector<std::string> inputs = {"Segment", "Position", "Tokens", "Mask"};
  std::vector<std::string> outputs;

  explicit BERTInterface(BERTConfig<TensorType> const &config)
  {
    outputs.emplace_back("norm_embed");
    for (SizeType i = 0; i < config.n_encoder_layers; i++)
    {
      outputs.emplace_back("SelfAttentionEncoder_No_" + std::to_string(i));
    }
  }
};

template <class TensorType>
std::pair<std::vector<std::string>, std::vector<std::string>> MakeBertModel(
    BERTConfig<TensorType> const &config, fetch::ml::Graph<TensorType> &g);

template <class TensorType>
void EvaluateGraph(fetch::ml::Graph<TensorType> &g, std::vector<std::string> input_nodes,
                   std::string const &output_node, std::vector<TensorType> input_data,
                   TensorType output_data, bool verbose = true);

template <class TensorType>
TensorType LoadTensorFromFile(std::string const &file_name);

template <class TensorType>
void PutWeightInLayerNorm(fetch::ml::StateDict<TensorType> &state_dict, SizeType model_dims,
                          std::string const &gamma_file_name, std::string const &beta_file_name,
                          std::string const &gamma_weight_name, std::string const &beta_weight_name);

template <class TensorType>
void PutWeightInFullyConnected(fetch::ml::StateDict<TensorType> &state_dict, SizeType in_size,
                               SizeType out_size, std::string const &weights_file_name,
                               std::string const &bias_file_name, std::string const &weights_name,
                               std::string const &bias_name);

template <class TensorType>
void PutWeightInMultiheadAttention(
    fetch::ml::StateDict<TensorType> &state_dict, SizeType n_heads, SizeType model_dims,
    std::string const &query_weights_file_name, std::string const &query_bias_file_name,
    std::string const &key_weights_file_name, std::string const &key_bias_file_name,
    std::string const &value_weights_file_name, std::string const &value_bias_file_name,
    std::string const &query_weights_name, std::string const &query_bias_name,
    std::string const &key_weights_name, std::string const &key_bias_name,
    std::string const &value_weights_name, std::string const &value_bias_name,
    std::string const &mattn_prefix);


template <class TensorType>
std::pair<std::vector<std::string>, std::vector<std::string>> LoadPretrainedBertModel(
    std::string const &file_path, BERTConfig<TensorType> const &config,
    fetch::ml::Graph<TensorType> &g);

template <class TensorType>
TensorType RunPseudoForwardPass(std::vector<std::string> input_nodes, std::string output_node,
                                BERTConfig<TensorType> const &config,
                                fetch::ml::Graph<TensorType> g, SizeType batch_size, bool verbose);

template <class TensorType>
std::vector<TensorType> PrepareTensorForBert(TensorType const &            data,
                                             BERTConfig<TensorType> const &config);
}  // namespace utilities
}  // namespace ml
}  // namespace fetch
