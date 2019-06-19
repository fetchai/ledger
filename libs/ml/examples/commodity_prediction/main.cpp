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
#include "math/tensor.hpp"
#include "ml/dataloaders/commodity_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/transpose.hpp"
#include "ml/state_dict.hpp"

#include <fstream>
#include <iostream>
#include <string>

using DataType  = double;
using ArrayType = fetch::math::Tensor<DataType>;
using GraphType = fetch::ml::Graph<ArrayType>;

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using namespace fetch::math;

enum class LayerType
{
  NONE,
  DENSE,
  DROPOUT,
  SOFTMAX
};

LayerType get_layer_type(std::string const &layer_name)
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
    throw std::runtime_error("Node name does not uniquely specify the node type.");
  }
  return layer_type;
}

/**
 * Loads a csv file into a Tensor
 * The Tensor will have the same number of rows as this file has (minus rows_to_skip) and the same
 * number of columns (minus cols_to_skip) as the file, unless transpose=true is specified in which
 * case it will be transposed.
 * @param filename  name of the file
 * @param cols_to_skip  number of columns to skip
 * @param rows_to_skip  number of rows to skip
 * @param transpose  whether to transpose the resulting Tensor
 * @return  Tensor with data
 */
ArrayType read_csv(std::string const &filename, SizeType const cols_to_skip = 0,
                   SizeType rows_to_skip = 0, bool transpose = false)
{
  std::ifstream file(filename);
  std::string   buf;

  // find number of rows and columns in the file
  std::string           delimiter = ",";
  SizeType              pos;
  fetch::math::SizeType row{0};
  fetch::math::SizeType col{0};
  while (std::getline(file, buf, '\n'))
  {
    while (row == 0 && ((pos = buf.find(delimiter)) != std::string::npos))
    {
      buf.erase(0, pos + delimiter.length());
      ++col;
    }
    ++row;
  }

  ArrayType weights({row - rows_to_skip, col + 1 - cols_to_skip});

  // read data into weights array
  std::string token;
  file.clear();
  file.seekg(0, std::ios::beg);

  while (rows_to_skip)
  {
    std::getline(file, buf, '\n');
    rows_to_skip--;
  }

  row = 0;
  while (std::getline(file, buf, '\n'))
  {
    col = 0;
    for (SizeType i = 0; i < cols_to_skip; i++)
    {
      pos = buf.find(delimiter);
      buf.erase(0, pos + delimiter.length());
    }
    while ((pos = buf.find(delimiter)) != std::string::npos)
    {
      weights(row, col) = static_cast<DataType>(std::stod(buf.substr(0, pos)));
      buf.erase(0, pos + delimiter.length());
      ++col;
    }
    weights(row, col) = static_cast<DataType>(std::stod(buf));
    ++row;
  }

  if (transpose)
  {
    weights = weights.Transpose();
  }
  return weights;
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
std::pair<std::string, std::vector<std::string>> read_architecture(
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

  LayerType layer_type;

  while (line_num--)  // continue reading until we get to the desired line
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

  g->AddNode<PlaceHolder<ArrayType>>(layer_name, {});  // add node for input

  // Iterate through fields adding nodes to graph
  while (previous_layer_name.find("output") == std::string::npos)
  {
    std::getline(ss, layer_name, delimiter);
    layer_type = get_layer_type(layer_name);

    switch (layer_type)
    {
    case LayerType::SOFTMAX:
    {
      previous_layer_name =
          g->AddNode<fetch::ml::ops::Softmax<ArrayType>>(layer_name, {previous_layer_name});
      break;
    }
    case LayerType::DROPOUT:
    {
      previous_layer_name =
          g->AddNode<Dropout<ArrayType>>(layer_name, {previous_layer_name}, dropout_prob);
      break;
    }
    case LayerType::DENSE:
    {
      ss >> layer_size >> delimiter;
      previous_layer_name = g->AddNode<FullyConnected<ArrayType>>(layer_name, {previous_layer_name},
                                                                  previous_layer_size, layer_size);
      previous_layer_size = layer_size;
      break;
    }
    default:
    {
      throw std::runtime_error("Unknown node type");
    }
    }

    node_names.emplace_back(previous_layer_name);
  }

  // there might be a final softmax layer
  std::getline(ss, layer_name, delimiter);
  layer_type = get_layer_type(layer_name);
  if (layer_type == LayerType::SOFTMAX)
  {
    previous_layer_name =
        g->AddNode<fetch::ml::ops::Softmax<ArrayType>>(layer_name, {previous_layer_name});
    node_names.emplace_back(previous_layer_name);
  }
  else
  {
    throw std::runtime_error("Unexpected node type");
  }

  return std::make_pair(dataname, node_names);
}

int ArgPos(char *str, int argc, char **argv)
{
  int a;
  for (a = 1; a < argc; a++)
  {
    if (!strcmp(str, argv[a]))
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

/**
 * Loads in a model, evaluates it on test inputs and compares this with test outputs.
 * Usage: -model_num 2 (line in the model file to read) and -input_dir (directory with
 * model weights and test files)
 */
int main(int argc, char **argv)
{
  int         i;
  std::string input_dir           = "";
  SizeType    model_num           = 0;
  SizeType    output_feature_size = 0;

  /// READ ARGUMENTS
  if ((i = ArgPos((char *)"-model_num", argc, argv)) > 0)
  {
    model_num = static_cast<SizeType>(std::stoul(argv[i + 1]));
  }
  if ((i = ArgPos((char *)"-input_dir", argc, argv)) > 0)
  {
    input_dir = argv[i + 1];
  }

  if (input_dir.empty())
  {
    throw std::runtime_error("Please specify an input directory");
  }

  std::string architecture_file = input_dir + "/architecture.csv";

  /// DEFINE NEURAL NET ARCHITECTURE ///

  auto g_ptr = std::make_shared<fetch::ml::Graph<ArrayType>>(GraphType());

  auto                     arch_tuple = read_architecture(architecture_file, g_ptr, model_num);
  std::string              dataname   = arch_tuple.first;
  std::vector<std::string> node_names = arch_tuple.second;

  std::string test_x_file = input_dir + "/" + dataname + "_x_test.csv";
  std::string test_y_file = input_dir + "/" + dataname + "_y_pred_test.csv";
  std::string weights_dir = input_dir + "/output/" + dataname + "/model_weights";

  /// LOAD WEIGHTS INTO GRAPH ///
  auto sd = g_ptr->StateDict();

  for (auto const &name : node_names)
  {
    if (name.find("dense") != std::string::npos)
    {
      // if it is a dense layer there will be weights and bias files
      std::vector<std::string> dir_list =
          fetch::ml::examples::GetAllTextFiles(weights_dir + "/" + name, "");
      std::vector<std::string> actual_dirs;
      for (auto dir : dir_list)
      {
        if (dir != "." && dir != "..")
        {
          actual_dirs.emplace_back(dir);
        }
      }
      assert(actual_dirs.size() == 1);

      std::string node_weights_dir = weights_dir + "/" + name + "/" + actual_dirs[0];
      // the weights array for the node has number of columns = number of features
      ArrayType weights = read_csv(node_weights_dir + "/kernel:0.csv", 0, 0, true);
      ArrayType bias    = read_csv(node_weights_dir + "/bias:0.csv", 0, 0, false);

      assert(bias.shape()[0] == weights.shape()[0]);

      auto weights_tensor = sd.dict_[name + "_FC_Weights"].weights_;
      auto bias_tensor    = sd.dict_[name + "_FC_Bias"].weights_;

      *weights_tensor     = weights;
      *bias_tensor        = bias;
      output_feature_size = weights.shape()[0];  // stores shape of last dense layer
    }
  }

  // load state dict into graph (i.e. load pretrained weights)
  g_ptr->LoadStateDict(sd);

  /// FORWARD PASS PREDICTIONS ///
  fetch::ml::CommodityDataLoader<fetch::math::Tensor<DataType>, fetch::math::Tensor<DataType>>
      loader;
  loader.AddData(test_x_file, test_y_file);

  ArrayType output({loader.Size(), output_feature_size});
  ArrayType test_y({loader.Size(), output_feature_size});

  SizeType j = 0;
  while (!loader.IsDone())
  {
    auto input = loader.GetNext();
    g_ptr->SetInput("num_input", input.second.at(0).Transpose());

    auto slice_output = g_ptr->Evaluate(node_names.back());
    output.Slice(j).Assign(slice_output);
    test_y.Slice(j).Assign(input.first);
    j++;
  }

  if (output.AllClose(test_y, 0.00001f))
  {
    std::cout << "Graph output is the same as the test output - success!";
  }
  else
  {
    std::cout << "Graph output is the different from the test output - fail.";
  }
}
