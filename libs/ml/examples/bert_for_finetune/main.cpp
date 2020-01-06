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
#include "math/tensor/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/slice.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/bert_utilities.hpp"
#include "ml/utilities/graph_saver.hpp"

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

// IMDB dataset loading and preparing
std::vector<TensorType> LoadIMDBFinetuneData(std::string const &file_path);
std::vector<std::pair<std::vector<TensorType>, TensorType>> PrepareIMDBFinetuneTrainData(
    std::string const &file_path, SizeType train_size, SizeType test_size,
    BERTConfig<TensorType> const &config);

int main(int ac, char **av)
{
  if (ac != 3)
  {
    std::cout << "Wrong number of / No available auguments" << std::endl;
    std::cout << ac;
    return 1;
  }

  // setup params for training
  SizeType train_size = 25;
  SizeType test_size  = 5;
  SizeType batch_size = 4;
  SizeType epochs     = 20;
  SizeType layer_no   = 12;
  auto     lr         = fetch::math::Type<DataType>("0.00001");
  // load data into memory
  std::string file_path = av[1];
  std::string IMDB_path = av[2];
  std::cout << "Pretrained BERT from folder: " << file_path << std::endl;
  std::cout << "IMDB review data: " << IMDB_path << std::endl;
  std::cout << "Starting FETCH BERT Demo" << std::endl;

  BERTConfig<TensorType> config{};
  // prepare IMDB data
  auto all_train_data = PrepareIMDBFinetuneTrainData(IMDB_path, train_size, test_size, config);

  // load pretrained bert model
  GraphType                 g = *(LoadGraph<GraphType>(file_path));
  BERTInterface<TensorType> bert_interface(config);
  std::cout << "finish loading pretraining model" << std::endl;

  std::vector<std::string> bert_inputs  = bert_interface.inputs;
  std::string              layer_output = bert_interface.outputs[layer_no];

  // Add linear classification layer
  std::string cls_token_output = g.template AddNode<fetch::ml::ops::Slice<TensorType>>(
      "ClsTokenOutput", {layer_output}, 0u, 1u);
  std::string classification_output =
      g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
          "ClassificationOutput", {cls_token_output}, config.model_dims, 2u,
          ActivationType::SOFTMAX, RegType::NONE, DataType{0}, WeightsInitType::XAVIER_GLOROT,
          false);

  // Set up error signal
  std::string label = g.template AddNode<PlaceHolder<TensorType>>("Label", {});
  std::string error =
      g.template AddNode<CrossEntropyLoss<TensorType>>("Error", {classification_output, label});
  std::cout << "finish creating cls model based on pretrain model" << std::endl;

  // output training stats
  std::cout << "output layer no: " << layer_no << std::endl;
  std::cout << "train_size: " << 2 * train_size << std::endl;
  std::cout << "batch_size: " << batch_size << std::endl;
  std::cout << "epochs: " << epochs << std::endl;
  std::cout << "lr: " << lr << std::endl;

  EvaluateGraph(g, bert_inputs, classification_output, all_train_data[1].first,
                all_train_data[1].second, true);

  // create optimizer
  std::cout << "START TRAINING" << std::endl;
  OptimiserType optimiser(std::make_shared<GraphType>(g), bert_inputs, label, error, lr);
  for (SizeType i = 0; i < epochs; i++)
  {
    DataType loss = optimiser.Run(all_train_data[0].first, all_train_data[0].second, batch_size);
    std::cout << "loss: " << loss << std::endl;
    EvaluateGraph(g, bert_inputs, classification_output, all_train_data[1].first,
                  all_train_data[1].second, true);
  }

  return 0;
}

std::vector<std::pair<std::vector<TensorType>, TensorType>> PrepareIMDBFinetuneTrainData(
    std::string const &file_path, SizeType train_size, SizeType test_size,
    BERTConfig<TensorType> const &config)
{
  // prepare IMDB data
  auto train_data = LoadIMDBFinetuneData(file_path);
  std::cout << "finish loading imdb from disk, start preprocessing" << std::endl;

  // evenly mix pos and neg train data together
  TensorType train_data_mixed({config.max_seq_len, static_cast<SizeType>(2) * train_size});
  for (SizeType i = 0; i < train_size; i++)
  {
    train_data_mixed.View(static_cast<SizeType>(2) * i).Assign(train_data[0].View(i));
    train_data_mixed.View(static_cast<SizeType>(2) * i + 1).Assign(train_data[1].View(i));
  }
  auto final_train_data = PrepareTensorForBert(train_data_mixed, config);

  // prepare label for train data
  TensorType train_labels({static_cast<SizeType>(2), static_cast<SizeType>(2) * train_size});
  for (SizeType i = 0; i < train_size; i++)
  {
    train_labels.Set(static_cast<SizeType>(0), static_cast<SizeType>(2) * i, DataType{1});
    train_labels.Set(static_cast<SizeType>(1), static_cast<SizeType>(2) * i + 1, DataType{1});
  }

  // evenly mix pos and neg test data together
  TensorType test_data_mixed({config.max_seq_len, static_cast<SizeType>(2) * test_size});
  for (SizeType i = 0; i < test_size; i++)
  {
    test_data_mixed.View(static_cast<SizeType>(2) * i).Assign(train_data[2].View(i));
    test_data_mixed.View(static_cast<SizeType>(2) * i + 1).Assign(train_data[3].View(i));
  }
  auto final_test_data = PrepareTensorForBert(test_data_mixed, config);

  // prepare label for train data
  TensorType test_labels({static_cast<SizeType>(2), static_cast<SizeType>(2) * test_size});
  for (SizeType i = 0; i < test_size; i++)
  {
    test_labels.Set(static_cast<SizeType>(0), static_cast<SizeType>(2) * i, DataType{1});
    test_labels.Set(static_cast<SizeType>(1), static_cast<SizeType>(2) * i + 1, DataType{1});
  }
  std::cout << "finish preparing train test data" << std::endl;

  return {std::make_pair(final_train_data, train_labels),
          std::make_pair(final_test_data, test_labels)};
}

std::vector<TensorType> LoadIMDBFinetuneData(std::string const &file_path)
{
  TensorType train_pos = LoadTensorFromFile<TensorType>(file_path + "train_pos");
  TensorType train_neg = LoadTensorFromFile<TensorType>(file_path + "train_neg");
  TensorType test_pos  = LoadTensorFromFile<TensorType>(file_path + "test_pos");
  TensorType test_neg  = LoadTensorFromFile<TensorType>(file_path + "test_neg");
  return {train_pos, train_neg, test_pos, test_neg};
}
