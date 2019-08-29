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
#include "ml/optimisation/sgd_optimiser.hpp"

#include "core/filesystem/read_file_contents.hpp"
#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "math/metrics/cross_entropy.hpp"
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
// using OptimiserType = typename fetch::ml::optimisers::SGDOptimiser<TensorType>;

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

// load tensor functionalities
TensorType LoadTensorFromFile(std::string file_name);

// finetuning functions
std::vector<TensorType> LoadIMDBFinetuneData(std::string const &file_path);
std::vector<TensorType> PrepareTensorForBert(TensorType const &data, BERTConfig const &config);

void EvaluateGraph(GraphType &g, std::vector<std::string> input_nodes, std::string output_node,
                   std::vector<TensorType> input_data, TensorType output_data, bool verbose = true);

GraphType ReadFileToGraph(std::string const file_name);

std::vector<std::pair<std::vector<TensorType>, TensorType>> PrepareIMDBFinetuneTrainData(
    std::string const &file_path, SizeType train_size, SizeType test_size,
    BERTConfig const &config);

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
  DataType lr         = static_cast<DataType>(5e-5);
  // load data into memory
  std::string file_path = av[1];
  std::string IMDB_path = av[2];
  std::cout << "Pretrained BERT from folder: " << file_path << std::endl;
  std::cout << "IMDB review data: " << IMDB_path << std::endl;
  std::cout << "Starting FETCH BERT Demo" << std::endl;

  BERTConfig config;
  // prepare IMDB data
  auto all_train_data = PrepareIMDBFinetuneTrainData(IMDB_path, train_size, test_size, config);

  // load pretrained bert model
  GraphType     g = ReadFileToGraph(file_path);
  BERTInterface bertInterface(config);
  std::cout << "finish loading pretraining model" << std::endl;

  std::vector<std::string> bert_inputs  = bertInterface.inputs;
  std::string              layer_output = bertInterface.outputs[layer_no];

  // Add linear classification layer
  std::string cls_token_output = g.template AddNode<fetch::ml::ops::Slice<TensorType>>(
      "ClsTokenOutput", {layer_output}, 0u, 1u);
  std::string classification_output =
      g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
          "ClassificationOutput", {cls_token_output}, config.model_dims, 2u,
          ActivationType::SOFTMAX, RegType::NONE, static_cast<DataType>(0),
          WeightsInitType::XAVIER_GLOROT, false);

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

std::vector<std::pair<std::vector<TensorType>, TensorType>> PrepareIMDBFinetuneTrainData(
    std::string const &file_path, SizeType train_size, SizeType test_size, BERTConfig const &config)
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
    train_labels.Set(static_cast<SizeType>(0), static_cast<SizeType>(2) * i,
                     static_cast<DataType>(1));
    train_labels.Set(static_cast<SizeType>(1), static_cast<SizeType>(2) * i + 1,
                     static_cast<DataType>(1));
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
    test_labels.Set(static_cast<SizeType>(0), static_cast<SizeType>(2) * i,
                    static_cast<DataType>(1));
    test_labels.Set(static_cast<SizeType>(1), static_cast<SizeType>(2) * i + 1,
                    static_cast<DataType>(1));
  }
  std::cout << "finish preparing train test data" << std::endl;

  return {std::make_pair(final_train_data, train_labels),
          std::make_pair(final_test_data, test_labels)};
}

std::vector<TensorType> LoadIMDBFinetuneData(std::string const &file_path)
{
  TensorType train_pos = LoadTensorFromFile(file_path + "train_pos");
  TensorType train_neg = LoadTensorFromFile(file_path + "train_neg");
  TensorType test_pos  = LoadTensorFromFile(file_path + "test_pos");
  TensorType test_neg  = LoadTensorFromFile(file_path + "test_neg");
  return {train_pos, train_neg, test_pos, test_neg};
}

std::vector<TensorType> PrepareTensorForBert(TensorType const &data, BERTConfig const &config)
{
  SizeType max_seq_len = config.max_seq_len;
  // check that data shape is proper for bert input
  if (data.shape().size() != 2 || data.shape(0) != max_seq_len)
  {
    std::runtime_error("Incorrect data shape for given bert config");
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

TensorType LoadTensorFromFile(std::string file_name)
{
  std::ifstream weight_file(file_name);
  assert(weight_file.is_open());

  std::string weight_str;
  getline(weight_file, weight_str);
  weight_file.close();

  return TensorType::FromString(weight_str);
}

GraphType ReadFileToGraph(std::string const file_name)
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
