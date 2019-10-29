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

#include "file_loader.hpp"
#include "math/distance/cosine.hpp"
#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/ReadCSV.hpp"
#include "ml/dataloaders/commodity_dataloader.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/transpose.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/state_dict.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using DataType      = double;
using TensorType    = fetch::math::Tensor<DataType>;
using GraphType     = fetch::ml::Graph<TensorType>;
using OptimiserType = typename fetch::ml::optimisers::AdamOptimiser<TensorType>;

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using namespace fetch::math;

static const DataType LEARNING_RATE{0.1f};
static const SizeType EPOCHS{200};
static const SizeType BATCH_SIZE{64};
static const SizeType PATIENCE = 25;

enum class LayerType
{
  NONE,
  DENSE,
  DROPOUT,
  SOFTMAX
};

LayerType GetLayerType(std::string const &layer_name)
{
  LayerType layer_type    = LayerType::NONE;
  int       match_counter = 0;

  if (layer_name.find("dropout") != std::string::npos)
  {
    layer_type = LayerType::DROPOUT;
    match_counter++;
  }
  if (layer_name.find("dense") != std::string::npos)
  {
    layer_type = LayerType::DENSE;
    match_counter++;
  }
  if (layer_name.find("softmax") != std::string::npos)
  {
    layer_type = LayerType::SOFTMAX;
    match_counter++;
  }

  if (match_counter != 1)
  {
    throw fetch::ml::exceptions::InvalidInput("Node name does not uniquely specify the node type.");
  }
  return layer_type;
}

/**
 * Loads a single model architecture from a csv file and adds the specified nodes to the graph
 * Example csv line: {model_name},num_input,118,dropout_0,output_dense,54,softmax
 * Model_name needs to match that in the weights directory and the filenames of the x and y test
 * files, e.g. output/{model_name}/model_weights/hidden_dense_1/hidden_dense_1_12/bias:0.csv and
 * {model_name}_x_test.csv
 * File can contain several models, one per line.
 * @param filename name of the file
 * @param g Graph to add nodes to
 * @param line_num line in the architecture file to load
 * @return pair of the name of the data (e.g. keras_h7_aluminium_px_last_us) and a vector of the
 * names of the nodes
 */
std::pair<std::string, std::vector<std::string>> ReadArchitecture(
    std::string const &filename, std::shared_ptr<GraphType> const &g, SizeType line_num = 0)
{
  char                     delimiter = ',';
  std::ifstream            file(filename);
  std::string              buf;
  std::string              dataname;
  std::string              layer_name;
  std::string              previous_layer_name;
  SizeType                 layer_size;
  SizeType                 input_layer_size;
  SizeType                 previous_layer_size;
  std::string              layer_activation;
  DataType                 dropout_prob = 1.0;
  std::vector<std::string> node_names({});
  LayerType                layer_type;

  if (file.fail())
  {
    throw fetch::ml::exceptions::InvalidFile("ReadArchitecture cannot open file " + filename);
  }

  while ((line_num--) != 0u)  // continue reading until we get to the desired line
  {
    std::getline(file, buf, '\n');
  }

  std::getline(file, buf, '\n');

  std::stringstream ss(buf);
  std::getline(ss, dataname, delimiter);
  std::getline(ss, layer_name, delimiter);
  ss >> input_layer_size >> delimiter;
  previous_layer_size = input_layer_size;
  previous_layer_name = layer_name;
  node_names.emplace_back(previous_layer_name);

  // add input node
  g->AddNode<PlaceHolder<TensorType>>(layer_name, {});

  // Add label node
  std::string label_name = "num_label";
  g->AddNode<PlaceHolder<TensorType>>(label_name, {});
  node_names.push_back(label_name);

  // Iterate through fields adding nodes to graph
  while (previous_layer_name.find("output") == std::string::npos)
  {
    std::getline(ss, layer_name, delimiter);
    layer_type = GetLayerType(layer_name);

    switch (layer_type)
    {
    case LayerType::SOFTMAX:
    {
      previous_layer_name =
          g->AddNode<fetch::ml::ops::Softmax<TensorType>>(layer_name, {previous_layer_name});
      break;
    }
    case LayerType::DROPOUT:
    {
      ss >> dropout_prob >> delimiter;
      previous_layer_name =
          g->AddNode<Dropout<TensorType>>(layer_name, {previous_layer_name}, dropout_prob);
      break;
    }
    case LayerType::DENSE:
    {
      ss >> layer_size >> delimiter;
      previous_layer_name = g->AddNode<FullyConnected<TensorType>>(
          layer_name, {previous_layer_name}, previous_layer_size, layer_size);
      previous_layer_size = layer_size;
      break;
    }
    default:
    {
      throw fetch::ml::exceptions::InvalidInput("Unknown node type");
    }
    }

    node_names.emplace_back(previous_layer_name);
  }

  // there might be a final softmax layer
  std::getline(ss, layer_name, delimiter);
  layer_type = GetLayerType(layer_name);
  if (layer_type == LayerType::SOFTMAX)
  {
    previous_layer_name =
        g->AddNode<fetch::ml::ops::Softmax<TensorType>>(layer_name, {previous_layer_name});
    node_names.emplace_back(previous_layer_name);
  }
  else
  {
    throw fetch::ml::exceptions::InvalidInput("Unexpected node type");
  }

  // Add loss function
  std::string error_output = g->AddNode<fetch::ml::ops::MeanSquareErrorLoss<TensorType>>(
      "num_error", {layer_name, label_name});
  node_names.emplace_back(error_output);

  return std::make_pair(dataname, node_names);
}

int ArgPos(char const *str, int argc, char **argv)
{
  int a;
  for (a = 1; a < argc; a++)
  {
    if (strcmp(str, argv[a]) == 0)
    {
      if (a == argc - 1)
      {
        printf("Argument missing for %s\n", str);
        exit(1);
      }
      return a;
    }
  }
  return -1;
}

DataType get_loss(std::shared_ptr<GraphType> const &g_ptr, std::string const &test_x_file,
                  std::string const &test_y_file, std::vector<std::string> node_names)
{
  DataType                                                            loss         = 0;
  DataType                                                            loss_counter = 0;
  fetch::ml::dataloaders::CommodityDataLoader<TensorType, TensorType> loader;

  auto data  = fetch::ml::dataloaders::ReadCSV<TensorType>(test_x_file);
  auto label = fetch::ml::dataloaders::ReadCSV<TensorType>(test_y_file);
  loader.AddData(data, label);

  while (!loader.IsDone())
  {
    auto input = loader.GetNext();
    g_ptr->SetInput(node_names.at(1), input.first);
    g_ptr->SetInput(node_names.front(), input.second.at(0));

    auto loss_tensor = g_ptr->Evaluate(node_names.back(), false);
    loss += *(loss_tensor.begin());
    loss_counter++;
  }

  return loss / loss_counter;
}

/**
 * Loads in a model, evaluates it on test inputs and compares this with test outputs.
 * Usage: -model_num 2 (line in the model file to read) and -input_dir (directory with
 * model weights and test files)
 */
int main(int argc, char **argv)
{
  int         i;
  std::string input_dir;
  SizeType    model_num           = 0;
  SizeType    output_feature_size = 0;
  bool        testing             = false;

  /// READ ARGUMENTS
  if ((i = ArgPos("-model_num", argc, argv)) > 0)
  {
    model_num = static_cast<SizeType>(std::stoul(argv[i + 1]));
  }
  if ((i = ArgPos("-input_dir", argc, argv)) > 0)
  {
    input_dir = argv[i + 1];
  }
  if ((i = ArgPos("-testing", argc, argv)) > 0)
  {
    testing = static_cast<bool>(std::stoi(argv[i + 1]));
  }

  if (input_dir.empty())
  {
    throw fetch::ml::exceptions::InvalidInput("Please specify an input directory");
  }

  std::string architecture_file = input_dir + "/architecture.csv";

  /// DEFINE NEURAL NET ARCHITECTURE ///

  auto g_ptr = std::make_shared<fetch::ml::Graph<TensorType>>(GraphType());

  auto                     arch_tuple = ReadArchitecture(architecture_file, g_ptr, model_num);
  std::string              dataname   = arch_tuple.first;
  std::vector<std::string> node_names = arch_tuple.second;

  std::string filename_root = input_dir + "/" + dataname + "_";

  if (testing)
  {
    /// LOAD WEIGHTS INTO GRAPH ///
    std::string weights_dir = input_dir + "/output/" + dataname + "/model_weights";
    auto        sd          = g_ptr->StateDict();

    for (auto const &name : node_names)
    {
      if (name.find("dense") != std::string::npos)
      {
        // if it is a dense layer there will be weights and bias files
        std::vector<std::string> dir_list =
            fetch::ml::examples::GetAllTextFiles(weights_dir.append("/").append(name), "");
        std::vector<std::string> actual_dirs;
        for (auto const &dir : dir_list)
        {
          if (dir != "." && dir != "..")
          {
            actual_dirs.emplace_back(dir);
          }
        }
        assert(actual_dirs.size() == 1);

        std::string node_weights_dir =
            weights_dir.append("/").append(name).append("/").append(actual_dirs[0]);

        // the weights array for the node has number of columns = number of features
        TensorType weights = fetch::ml::dataloaders::ReadCSV<TensorType>(
            node_weights_dir + "/kernel:0.csv", 0, 0, true);
        TensorType bias = fetch::ml::dataloaders::ReadCSV<TensorType>(
            node_weights_dir + "/bias:0.csv", 0, 0, false);

        assert(bias.shape().at(0) == weights.shape().at(0));

        auto weights_tensor = sd.dict_.at(name + "_FullyConnected_Weights").weights_;
        auto bias_tensor    = sd.dict_.at(name + "_FullyConnected_Bias").weights_;

        *weights_tensor     = weights;
        *bias_tensor        = bias;
        output_feature_size = weights.shape()[0];  // stores shape of last dense layer
      }
    }

    // load state dict into graph (i.e. load pretrained weights)
    g_ptr->LoadStateDict(sd);

    /// LOAD DATA ///

    std::string test_x_file = filename_root + "x_test.csv";
    std::string test_y_file = filename_root + "y_pred_test.csv";
    fetch::ml::dataloaders::CommodityDataLoader<TensorType, TensorType> loader;
    auto data  = fetch::ml::dataloaders::ReadCSV<TensorType>(test_x_file);
    auto label = fetch::ml::dataloaders::ReadCSV<TensorType>(test_y_file);
    loader.AddData(data, label);

    /// FORWARD PASS PREDICTIONS ///

    TensorType output({loader.Size(), output_feature_size});
    TensorType test_y({loader.Size(), output_feature_size});

    SizeType j = 0;
    while (!loader.IsDone())
    {
      auto input = loader.GetNext();
      g_ptr->SetInput("num_input", input.second.at(0));

      auto slice_output = g_ptr->Evaluate(node_names.at(node_names.size() - 2), false);
      output.Slice(j).Assign(slice_output);
      test_y.Slice(j).Assign(input.first);
      j++;
    }

    if (output.AllClose(test_y, 0.00001f))
    {
      std::cout << "Graph output is the same as the test output - success!" << std::endl;
    }
    else
    {
      std::cout << "Graph output is the different from the test output - fail." << std::endl;
    }
  }
  else
  {
    /// TRAIN ///
    bool     use_random = false;
    DataType loss{0};

    // Initialise Optimiser
    OptimiserType optimiser(g_ptr, {node_names.front()}, node_names.at(1), node_names.back(),
                            LEARNING_RATE);

    fetch::ml::dataloaders::CommodityDataLoader<TensorType, TensorType> loader;

    // three training rounds
    for (SizeType j = 0; j < 3; j++)
    {
      std::cout << std::endl << "Starting training loop " << j << std::endl;

      std::string train_x_file;

      if (use_random)
      {
        train_x_file = filename_root + "random_" + std::to_string(j) + "_x_train.csv";
      }
      else
      {
        train_x_file = filename_root + std::to_string(j) + "_x_train.csv";
      }
      std::string train_y_file = filename_root + std::to_string(j) + "_y_train.csv";
      std::string test_x_file  = filename_root + std::to_string(j) + "_x_test.csv";
      std::string test_y_file  = filename_root + std::to_string(j) + "_y_test.csv";
      std::string valid_x_file = filename_root + std::to_string(j) + "_x_val.csv";
      std::string valid_y_file = filename_root + std::to_string(j) + "_y_val.csv";

      loader.Reset();
      auto data  = fetch::ml::dataloaders::ReadCSV<TensorType>(train_x_file);
      auto label = fetch::ml::dataloaders::ReadCSV<TensorType>(train_y_file);
      loader.AddData(data, label);

      // Training loop
      // run first loop to get loss

      DataType min_loss       = numeric_max<DataType>();
      SizeType patience_count = 0;

      for (SizeType k{0}; k < EPOCHS; k++)
      {
        loss = optimiser.Run(loader, BATCH_SIZE);
        std::cout << "Training Loss: " << loss << std::endl;

        loss = get_loss(g_ptr, valid_x_file, valid_y_file, node_names);
        std::cout << "Validation loss: " << loss << std::endl;

        // update early stopping
        if (loss < min_loss)
        {
          min_loss       = loss;
          patience_count = 0;
        }
        else
        {
          patience_count++;
          if (patience_count >= PATIENCE)
          {
            std::cout << "Stopping early" << std::endl;
            break;
          }
        }
      }

      loss = get_loss(g_ptr, test_x_file, test_y_file, node_names);
      std::cout << "Testing loss: " << loss << std::endl;
    }
    std::cout << std::endl << "Finished training" << std::endl;

    /// Final testing ///
    std::string test_x_file = filename_root + "x_test.csv";
    std::string test_y_file = filename_root + "y_pred_test.csv";

    loader.Reset();
    auto data  = fetch::ml::dataloaders::ReadCSV<TensorType>(test_x_file);
    auto label = fetch::ml::dataloaders::ReadCSV<TensorType>(test_y_file);
    loader.AddData(data, label);

    DataType    distance         = 0;
    DataType    distance_counter = 0;
    std::string our_y_output = filename_root + "y_pred_test_fetch_" + std::to_string(EPOCHS) + "_" +
                               std::to_string(static_cast<int>(use_random)) + "_" +
                               std::to_string(LEARNING_RATE) + ".csv";

    bool          first = true;
    std::ofstream file(our_y_output);
    while (!loader.IsDone())
    {
      auto input = loader.GetNext();
      g_ptr->SetInput("num_input", input.second.at(0));

      auto slice_output = g_ptr->Evaluate(node_names.at(node_names.size() - 2), false);
      // write slice_output to csv
      if (first)
      {
        for (SizeType k = 0; k < slice_output.shape(0); k++)
        {
          file << "," << k;
        }
        file << std::endl;
        first = false;
      }

      file << distance_counter;
      for (SizeType k = 0; k < slice_output.shape(0); k++)
      {
        file << "," << slice_output[k];
      }
      file << std::endl;

      auto cos = fetch::math::correlation::Cosine(slice_output, input.first);
      distance += cos;
      distance_counter++;
    }
    file.close();

    std::cout << "Average cosine correlation: " << distance / distance_counter << std::endl;
  }

  return 0;
}
