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
  DataType dropout_keep_prob = static_cast<DataType>(1);
};

struct BERTInterface
{
  // the default names for input and outpus of a Fetch BERT model
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
    BERTConfig const &config, GraphType &g);

std::pair<std::vector<TensorType>, TensorType> PrepareToyClsDataset(SizeType          size,
                                                                    BERTConfig const &config,
                                                                    SizeType          seed = 1337);

void EvaluateGraph(GraphType &g, std::vector<std::string> input_nodes, std::string output_node,
                   std::vector<TensorType> input_data, TensorType output_data, bool verbose = true);

int main()
{
  // This example will create a simple bert-like model that can classify between single token input
  // and shuffled token input e.g. 0111111 will be classified as 1, 01121312 will be classified as
  // 0.
  std::cout << "FETCH BERT Toy CLS Demo" << std::endl;

  SizeType train_size = 1000;
  SizeType test_size  = 100;
  SizeType batch_size = 16;
  SizeType epochs     = 2;
  DataType lr         = static_cast<DataType>(1e-3);

  BERTConfig config;
  config.n_encoder_layers  = 2u;
  config.max_seq_len       = 20u;
  config.model_dims        = 12u;
  config.n_heads           = 2u;
  config.ff_dims           = 12u;
  config.vocab_size        = 4u;
  config.segment_size      = 1u;
  config.dropout_keep_prob = static_cast<DataType>(0.9);

  BERTInterface interface(config);

  // create custom bert model
  GraphType g;
  MakeBertModel(config, g);

  // Add linear classification layer
  std::string cls_token_output = g.template AddNode<fetch::ml::ops::Slice<TensorType>>(
      "ClsTokenOutput", {interface.outputs[interface.outputs.size() - 1]}, 0u, 1u);
  std::string classification_output =
      g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
          "ClassificationOutput", {cls_token_output}, config.model_dims, 2u,
          ActivationType::SOFTMAX, RegType::NONE, static_cast<DataType>(0),
          WeightsInitType::XAVIER_GLOROT, false);
  // Set up error signal
  std::string label = g.template AddNode<PlaceHolder<TensorType>>("Label", {});
  std::string error =
      g.template AddNode<CrossEntropyLoss<TensorType>>("Error", {classification_output, label});

  // Do pre-validation
  auto test_data = PrepareToyClsDataset(test_size, config, static_cast<SizeType>(1));
  EvaluateGraph(g, interface.inputs, classification_output, test_data.first, test_data.second);

  // Do training
  auto          train_data = PrepareToyClsDataset(train_size, config, static_cast<SizeType>(0));
  OptimiserType optimiser(std::make_shared<GraphType>(g), interface.inputs, label, error, lr);

  for (SizeType i = 0; i < epochs; i++)
  {
    optimiser.Run(train_data.first, train_data.second, batch_size);
    EvaluateGraph(g, interface.inputs, classification_output, test_data.first, test_data.second,
                  false);
  }

  // Do validation
  EvaluateGraph(g, interface.inputs, classification_output, test_data.first, test_data.second);

  return 0;
}

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

  std::cout << max_seq_len << std::endl;

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

std::pair<std::vector<TensorType>, TensorType> PrepareToyClsDataset(SizeType          size,
                                                                    BERTConfig const &config,
                                                                    SizeType          seed)
{
  // create a toy cls dataset that generated balanced training data for the aforementioned
  // classification task
  TensorType tokens_data({config.max_seq_len, size});
  TensorType labels({static_cast<SizeType>(2), size});

  fetch::random::LaggedFibonacciGenerator<> lfg(seed);

  for (SizeType i = 0; i < size; i++)
  {
    // 0 is used as cls token in this dataset
    tokens_data.Set(0, i, static_cast<DataType>(0));
    if (i % 2 == 0)
    {  // get one constant token in the library other then 0 (the cls token)
      DataType token = static_cast<DataType>(1 + lfg() % (config.vocab_size - 1));
      for (SizeType entry = 1; entry < config.max_seq_len; entry++)
      {
        tokens_data.Set(entry, i, token);
      }
      labels.Set(0u, i, static_cast<DataType>(1));  // label 1 0
    }
    else
    {  // interval tokens_data
      for (SizeType entry = 1; entry < config.max_seq_len; entry++)
      {
        // get a random different token for each position of the tokens_data
        DataType token = static_cast<DataType>(1 + lfg() % (config.vocab_size - 1));
        tokens_data.Set(entry, i, token);
      }
      labels.Set(1u, i, static_cast<DataType>(1));  // label 0 1
    }
  }

  // augment token input to be bert inputs
  std::vector<TensorType> final_data;
  TensorType              segment_data({config.max_seq_len, size});
  final_data.emplace_back(segment_data);
  TensorType position_data({config.max_seq_len, size});
  final_data.emplace_back(position_data);
  TensorType mask_data({config.max_seq_len, 1, size});
  final_data.emplace_back(tokens_data);
  mask_data.Fill(static_cast<DataType>(1));
  final_data.emplace_back(mask_data);

  return std::make_pair(final_data, labels);
}

void EvaluateGraph(GraphType &g, std::vector<std::string> input_nodes, std::string output_node,
                   std::vector<TensorType> input_data, TensorType output_data, bool verbose)
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
