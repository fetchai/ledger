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
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/mean_square_error.hpp"
#include "vm/module.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/dataloaders/commodity_dataloader.hpp"
#include "vm_modules/core/print.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using DataType  = float;  // todo: setting this to double breaks Arch
using ArrayType = fetch::math::Tensor<DataType>;
using GraphType = fetch::ml::Graph<ArrayType>;

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using namespace fetch::math;

struct System : public fetch::vm::Object
{
  System()          = delete;
  virtual ~System() = default;

  static int32_t Argc(fetch::vm::VM * /*vm*/, fetch::vm::TypeId /*type_id*/)
  {
    return int32_t(System::args.size());
  }

  static fetch::vm::Ptr<fetch::vm::String> Argv(fetch::vm::VM *vm, fetch::vm::TypeId /*type_id*/,
                                                int32_t const &a)
  {
    return fetch::vm::Ptr<fetch::vm::String>(
        new fetch::vm::String(vm, System::args[std::size_t(a)]));
  }

  static std::vector<std::string> args;
};

std::vector<std::string> System::args;

//enum class LayerType
//{
//  NONE,
//  DENSE,
//  DROPOUT,
//  SOFTMAX
//};
//
//LayerType get_layer_type(std::string const &layer_name)
//{
//  LayerType layer_type    = LayerType::NONE;
//  int       match_counter = 0;
//
//  if (layer_name.find("dropout") != std::string::npos)
//  {
//    layer_type = LayerType::DROPOUT;
//    match_counter++;
//  }
//  if (layer_name.find("dense") != std::string::npos)
//  {
//    layer_type = LayerType::DENSE;
//    match_counter++;
//  }
//  if (layer_name.find("softmax") != std::string::npos)
//  {
//    layer_type = LayerType::SOFTMAX;
//    match_counter++;
//  }
//
//  if (match_counter != 1)
//  {
//    throw std::runtime_error("Node name does not uniquely specify the node type.");
//  }
//  return layer_type;
//}
//
//
///**
// * Loads a single model architecture from a csv file and adds the specified nodes to the graph
// * Example csv line: {model_name},num_input,118,dropout_0,output_dense,54,softmax
// * Model_name needs to match that in the weights directory and the filenames of the x and y test
// * files, e.g. output/{model_name}/model_weights/hidden_dense_1/hidden_dense_1_12/bias:0.csv and
// * {model_name}_x_test.csv
// * File can contain several models, one per line.
// * @param filename name of the file
// * @param g Graph to add nodes to
// * @param line_num line in the architecture file to load
// * @return pair of the name of the data (e.g. keras_h7_aluminium_px_last_us) and a vector of the
// * names of the nodes
// */
//std::pair<std::string, std::vector<std::string>> read_architecture(
//    std::string const &filename, std::shared_ptr<GraphType> const &g, SizeType line_num = 0)
//{
//  char                     delimiter = ',';
//  std::ifstream            file(filename);
//  std::string              buf;
//  std::string              dataname;
//  std::string              layer_name;
//  std::string              previous_layer_name;
//  SizeType                 layer_size;
//  SizeType                 input_layer_size;
//  SizeType                 previous_layer_size;
//  std::string              layer_activation;
//  DataType                 dropout_prob = 1.0;
//  std::vector<std::string> node_names({});
//
//  LayerType layer_type;
//
//  while (line_num--)  // continue reading until we get to the desired line
//  {
//    std::getline(file, buf, '\n');
//  }
//
//  std::getline(file, buf, '\n');
//
//  std::stringstream ss(buf);
//  std::getline(ss, dataname, delimiter);
//  std::getline(ss, layer_name, delimiter);
//  ss >> input_layer_size >> delimiter;
//  previous_layer_size = input_layer_size;
//  previous_layer_name = layer_name;
//
//  g->AddNode<PlaceHolder<ArrayType>>(layer_name, {});  // add node for input
//
//  // Iterate through fields adding nodes to graph
//  while (previous_layer_name.find("output") == std::string::npos)
//  {
//    std::getline(ss, layer_name, delimiter);
//    layer_type = get_layer_type(layer_name);
//
//    switch (layer_type)
//    {
//      case LayerType::SOFTMAX:
//      {
//        previous_layer_name =
//            g->AddNode<fetch::ml::ops::Softmax<ArrayType>>(layer_name, {previous_layer_name});
//        break;
//      }
//      case LayerType::DROPOUT:
//      {
//        previous_layer_name =
//            g->AddNode<Dropout<ArrayType>>(layer_name, {previous_layer_name}, dropout_prob);
//        break;
//      }
//      case LayerType::DENSE:
//      {
//        ss >> layer_size >> delimiter;
//        previous_layer_name = g->AddNode<FullyConnected<ArrayType>>(layer_name, {previous_layer_name},
//            previous_layer_size, layer_size);
//        previous_layer_size = layer_size;
//        break;
//      }
//      default:
//      {
//        throw std::runtime_error("Unknown node type");
//      }
//    }
//
//    node_names.emplace_back(previous_layer_name);
//  }
//
//  // there might be a final softmax layer
//  std::getline(ss, layer_name, delimiter);
//  layer_type = get_layer_type(layer_name);
//  if (layer_type == LayerType::SOFTMAX)
//  {
//    previous_layer_name =
//        g->AddNode<fetch::ml::ops::Softmax<ArrayType>>(layer_name, {previous_layer_name});
//    node_names.emplace_back(previous_layer_name);
//  }
//  else
//  {
//    throw std::runtime_error("Unexpected node type");
//  }
//
//  return std::make_pair(dataname, node_names);
//}
//
//
//static void Arch(fetch::vm::VM * /*vm*/,
//                 fetch::vm::Ptr<fetch::vm::String> const & filename,
//                 fetch::vm::Ptr<fetch::vm_modules::ml::VMGraph> const & graph
//                 )
//{
//
//  auto g_ptr = std::make_shared<fetch::ml::Graph<ArrayType>>(graph->graph_);
//  read_architecture(filename->str, g_ptr);
//}

// read the weights and bias csv files

fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> read_csv(
    fetch::vm::VM *vm,
    fetch::vm::Ptr<fetch::vm::String> const &filename,
    bool transpose = false)
{
  std::ifstream file(filename->str);
  std::string   buf;

  // find number of rows and columns in the file
  char delimiter = ',';
  std::string field_value;
  fetch::math::SizeType row{0};
  fetch::math::SizeType col{0};

  while (std::getline(file, buf, '\n'))
  {
    std::stringstream ss(buf);

    while (row == 0 && std::getline(ss, field_value, delimiter))
    {
      ++col;
    }
    ++row;
  }

  ArrayType weights({row, col});

  // read data into weights array
  file.clear();
  file.seekg(0, std::ios::beg);

  row = 0;
  while (std::getline(file, buf, '\n'))
  {
    col = 0;
    std::stringstream ss(buf);
    while (std::getline(ss, field_value, delimiter))
    {
      weights(row, col) = static_cast<DataType>(std::stod(field_value));
      ++col;
    }
    ++row;
  }

  if (transpose)
  {
    weights = weights.Transpose();
  }

  return vm->CreateNewObject<fetch::vm_modules::math::VMTensor>(weights);
}


int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cerr << "usage ./" << argv[0] << " [filename]" << std::endl;
    std::exit(-9);
  }

  for (int i = 2; i < argc; ++i)
  {
    System::args.push_back(std::string(argv[i]));
  }

  // Reading file
  std::ifstream      file(argv[1], std::ios::binary);
  std::ostringstream ss;
  ss << file.rdbuf();
  const std::string source = ss.str();
  file.close();

  auto module = std::make_shared<fetch::vm::Module>();

  module->CreateClassType<System>("System")
      .CreateStaticMemberFunction("Argc", &System::Argc)
      .CreateStaticMemberFunction("Argv", &System::Argv);

  fetch::vm_modules::math::CreateTensor(*module);
  fetch::vm_modules::ml::CreateGraph(*module);
  fetch::vm_modules::ml::VMCommodityDataLoader::Bind(*module);
  fetch::vm_modules::CreatePrint(*module);

//  module->CreateFreeFunction("read_architecture", &Arch);
  module->CreateFreeFunction("read_csv", &read_csv);

  // Setting compiler up
  fetch::vm::Compiler *    compiler = new fetch::vm::Compiler(module.get());
  fetch::vm::Executable    executable;
  fetch::vm::IR            ir;
  std::vector<std::string> errors;

  // Compiling
  bool compiled = compiler->Compile(source, "myexecutable", ir, errors);

  if (!compiled)
  {
    std::cout << "Failed to compile" << std::endl;
    for (auto &s : errors)
    {
      std::cout << s << std::endl;
    }
    return -1;
  }

  fetch::vm::VM vm(module.get());

  // attach std::cout for printing
  vm.AttachOutputDevice("stdout", std::cout);

  if (!vm.GenerateExecutable(ir, "main_ir", executable, errors))
  {
    std::cout << "Failed to generate executable" << std::endl;
    for (auto &s : errors)
    {
      std::cout << s << std::endl;
    }
    return -1;
  }

  if (!executable.FindFunction("main"))
  {
    std::cout << "Function 'main' not found" << std::endl;
    return -2;
  }

  // Setting VM up and running
  std::string        error;
  fetch::vm::Variant output;

  if (!vm.Execute(executable, "main", error, output))
  {
    std::cout << "Runtime error on line " << error << std::endl;
  }
  delete compiler;
  return 0;
}
