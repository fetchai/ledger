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

#include "math/tensor/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/slice.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/utilities/bert_utilities.hpp"

#include <iostream>
#include <string>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using namespace fetch::ml::utilities;

using DataType   = float;
using TensorType = fetch::math::Tensor<DataType>;
using SizeVector = typename TensorType::SizeVector;

using GraphType     = typename fetch::ml::Graph<TensorType>;
using StateDictType = typename fetch::ml::StateDict<TensorType>;
using OptimiserType = typename fetch::ml::optimisers::AdamOptimiser<TensorType>;

using RegType         = fetch::ml::RegularisationType;
using WeightsInitType = fetch::ml::ops::WeightsInitialisation;
using ActivationType  = fetch::ml::details::ActivationType;

std::pair<std::vector<TensorType>, TensorType> PrepareToyClsDataset(
    SizeType size, BERTConfig<TensorType> const &config, SizeType seed = 1337);

int main()
{
  // This example will create a simple bert-like model that can classify between single token input
  // and shuffled token input e.g. 0111111 will be classified as 1, 01121312 will be classified as
  // 0.
  std::cout << "FETCH bert Toy CLS Demo" << std::endl;

  SizeType   train_size = 1000;
  SizeType   test_size  = 100;
  SizeType   batch_size = 16;
  SizeType   epochs     = 2;
  auto const lr         = fetch::math::Type<DataType>("0.001");

  BERTConfig<TensorType> config;
  config.n_encoder_layers  = 2u;
  config.max_seq_len       = 20u;
  config.model_dims        = 12u;
  config.n_heads           = 2u;
  config.ff_dims           = 12u;
  config.vocab_size        = 4u;
  config.segment_size      = 1u;
  config.dropout_keep_prob = fetch::math::Type<DataType>("0.9");

  BERTInterface<TensorType> interface(config);

  // create custom bert model
  GraphType g;
  MakeBertModel(config, g);

  // Add linear classification layer
  std::string cls_token_output = g.template AddNode<fetch::ml::ops::Slice<TensorType>>(
      "ClsTokenOutput", {interface.outputs[interface.outputs.size() - 1]}, 0u, 1u);
  std::string classification_output =
      g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
          "ClassificationOutput", {cls_token_output}, config.model_dims, 2u,
          ActivationType::SOFTMAX, RegType::NONE, DataType{0}, WeightsInitType::XAVIER_GLOROT,
          false);
  // Set up error signal
  std::string label = g.template AddNode<PlaceHolder<TensorType>>("Label", {});
  std::string error =
      g.template AddNode<CrossEntropyLoss<TensorType>>("Error", {classification_output, label});

  // Do pre-validation
  auto test_data = PrepareToyClsDataset(test_size, config, static_cast<SizeType>(1));
  EvaluateGraph(g, interface.inputs, classification_output, test_data.first, test_data.second,
                true);

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
  EvaluateGraph(g, interface.inputs, classification_output, test_data.first, test_data.second,
                true);

  return 0;
}

std::pair<std::vector<TensorType>, TensorType> PrepareToyClsDataset(
    SizeType size, BERTConfig<TensorType> const &config, SizeType seed)
{
  // create a toy cls dataset that generated balanced training data for the aforementioned
  // classification task
  TensorType tokens_data({config.max_seq_len, size});
  TensorType labels({static_cast<SizeType>(2), size});

  fetch::random::LaggedFibonacciGenerator<> lfg(seed);

  for (SizeType i = 0; i < size; i++)
  {
    // 0 is used as cls token in this dataset
    tokens_data.Set(0, i, DataType{0});
    if (i % 2 == 0)
    {  // get one constant token in the library other then 0 (the cls token)
      auto token = static_cast<DataType>(1 + lfg() % (config.vocab_size - 1));
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
        auto token = static_cast<DataType>(1 + lfg() % (config.vocab_size - 1));
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
