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

struct System : public fetch::vm::Object
{
  System()           = delete;
  ~System() override = default;

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
  ArrayType weights = fetch::ml::dataloaders::ReadCSV<ArrayType>(filename->str, 0, 0, transpose);
  return vm->CreateNewObject<fetch::vm_modules::math::VMTensor>(weights);
}

int main(int argc, char **argv)
{
  if (argc < 5)
  {
    std::cerr << "usage ./" << argv[0] << " [filename]" << std::endl;
    std::exit(-9);
  }

  for (int i = 2; i < argc; ++i)
  {
    System::args.emplace_back(std::string(argv[i]));
  }

  // Reading file
  std::ifstream file(argv[1], std::ios::binary);
  if (file.fail())
  {
    throw std::runtime_error("Cannot open file " + std::string(argv[1]));
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  const std::string source = ss.str();
  file.close();

  auto module = std::make_shared<fetch::vm::Module>();

  module->CreateClassType<System>("System")
      .CreateStaticMemberFunction("Argc", &System::Argc)
      .CreateStaticMemberFunction("Argv", &System::Argv);

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
