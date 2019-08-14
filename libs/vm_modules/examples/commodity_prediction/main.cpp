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
#include "ml/dataloaders/ReadCSV.hpp"
#include "vm/module.hpp"
#include "vm_modules/core/print.hpp"
#include "vm_modules/core/system.hpp"
#include "vm_modules/ml/ml.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using DataType  = typename fetch::vm_modules::math::VMTensor::DataType;
using ArrayType = fetch::math::Tensor<DataType>;
using System    = fetch::vm_modules::System;

// read the weights and bias csv files

fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> read_csv(
    fetch::vm::VM *vm, fetch::vm::Ptr<fetch::vm::String> const &filename, bool transpose = false)
{
  ArrayType weights = fetch::ml::dataloaders::ReadCSV<ArrayType>(filename->str, 0, 0, transpose);
  return vm->CreateNewObject<fetch::vm_modules::math::VMTensor>(weights);
}

int main(int argc, char **argv)
{
  // parse the command line parameters
  System::Parse(argc, argv);

  fetch::commandline::ParamsParser const &pp = System::GetParamParser();

  // ensure the program has the correct number of args
  if (2u != pp.arg_size())
  {
    std::cerr << "Usage: " << pp.GetArg(0) << " <filename> -- [script args]..." << std::endl;
    return 1;
  }

  // Reading file
  std::ifstream file(pp.GetArg(1), std::ios::binary);
  if (file.fail())
  {
    throw std::runtime_error("Cannot open file " + std::string(pp.GetArg(1)));
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  const std::string source = ss.str();
  file.close();

  auto module = std::make_shared<fetch::vm::Module>();

  fetch::vm_modules::System::Bind(*module);

  fetch::vm_modules::ml::BindML(*module);

  fetch::vm_modules::CreatePrint(*module);

  module->CreateFreeFunction("read_csv", &read_csv);

  // Setting compiler up
  auto                     compiler = std::make_unique<fetch::vm::Compiler>(module.get());
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
  vm.AttachOutputDevice(fetch::vm::VM::STDOUT, std::cout);

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
