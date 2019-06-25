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
#include "vm/module.hpp"
#include "vm_modules/core/print.hpp"
#include "vm_modules/ml/dataloaders/commodity_dataloader.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/training_pair.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using DataType  = typename fetch::vm_modules::math::VMTensor::DataType;
using ArrayType = fetch::math::Tensor<DataType>;

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

// read the weights and bias csv files

fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> read_csv(
    fetch::vm::VM *vm, fetch::vm::Ptr<fetch::vm::String> const &filename, bool transpose = false)
{
  std::ifstream file(filename->str);
  std::string   buf;

  // find number of rows and columns in the file
  char                  delimiter = ',';
  std::string           field_value;
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
    System::args.emplace_back(std::string(argv[i]));
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
  fetch::vm_modules::ml::VMStateDict::Bind(*module);
  fetch::vm_modules::ml::VMGraph::Bind(*module);
  fetch::vm_modules::ml::TrainingPair::Bind(*module);
  fetch::vm_modules::ml::VMCommodityDataLoader::Bind(*module);
  fetch::vm_modules::CreatePrint(*module);

  module->CreateFreeFunction("read_csv", &read_csv);

  // Setting compiler up
  auto compiler = std::make_unique<fetch::vm::Compiler>(module.get());
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
  return 0;
}
