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
#include "vm_modules/ml/cross_entropy.hpp"

#include "vm_modules/math/matrix_operations.hpp"
#include "vm_modules/ml/graph.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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

class TrainingPairWrapper : public fetch::vm::Object,
                            public std::pair<fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper>,
                                             fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper>>
{
public:
  TrainingPairWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                      fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> ta,
                      fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> tb)
    : fetch::vm::Object(vm, type_id)
  {
    this->first  = ta;
    this->second = tb;
  }

  static fetch::vm::Ptr<TrainingPairWrapper> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> ta,
      fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> tb)
  {
    return new TrainingPairWrapper(vm, type_id, ta, tb);
  }

  fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> data()
  {
    return this->second;
  }

  fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> label()
  {
    return this->first;
  }
};

class DataLoaderWrapper : public fetch::vm::Object
{
public:
  DataLoaderWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::string const &images_file,
                    std::string const &labels_file)
    : fetch::vm::Object(vm, type_id)
    , loader_(images_file, labels_file)
  {}

  static fetch::vm::Ptr<DataLoaderWrapper> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm::String> const &images_file,
      fetch::vm::Ptr<fetch::vm::String> const &labels_file)
  {
    return new DataLoaderWrapper(vm, type_id, images_file->str, labels_file->str);
  }

  // Wont compile if parameter is not const &
  // The actual fetch::vm::Ptr is const, but the pointed to memory is modified
  fetch::vm::Ptr<TrainingPairWrapper> GetData(fetch::vm::Ptr<TrainingPairWrapper> const &dataHolder)
  {
    std::pair<fetch::math::Tensor<float>, std::vector<fetch::math::Tensor<float>>> d =
        loader_.GetNext();
    (*(dataHolder->first)).Copy(d.first);
    (*(dataHolder->second)).Copy(d.second.at(0));
    return dataHolder;
  }

  void Display(fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> const &d)
  {
    loader_.Display(*d);
  }

private:
  fetch::ml::MNISTLoader<fetch::math::Tensor<float>, fetch::math::Tensor<float>> loader_;
};

template <typename T>
static void PrintNumber(fetch::vm::VM * /*vm*/, T const &s)
{
  std::cout << s << std::endl;
}

static void Print(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  std::cout << s->str << std::endl;
}

fetch::vm::Ptr<fetch::vm::String> toString(fetch::vm::VM *vm, float const &a)
{
  fetch::vm::Ptr<fetch::vm::String> ret(new fetch::vm::String(vm, std::to_string(a)));
  return ret;
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

  module->CreateFreeFunction("print", &PrintNumber<int>);
  module->CreateFreeFunction("print", &PrintNumber<uint64_t>);
  module->CreateFreeFunction("print", &PrintNumber<float>);
  module->CreateFreeFunction("print", &PrintNumber<double>);
  module->CreateFreeFunction("print", &Print);
  module->CreateFreeFunction("toString", &toString);

  module->CreateClassType<System>("System")
      .CreateStaticMemberFunction("Argc", &System::Argc)
      .CreateStaticMemberFunction("Argv", &System::Argv);


  fetch::vm_modules::CreateArgMax(*module);
  fetch::vm_modules::ml::CreateTensor(*module);
  fetch::vm_modules::ml::CreateGraph(*module);
  fetch::vm_modules::ml::CreateCrossEntropy(*module);

  module->CreateClassType<TrainingPairWrapper>("TrainingPair")
      .CreateConstuctor<fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper>,
                        fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper>>()
      .CreateMemberFunction("Data", &TrainingPairWrapper::data)
      .CreateMemberFunction("Label", &TrainingPairWrapper::label);

  module->CreateClassType<DataLoaderWrapper>("MNISTLoader")
      .CreateConstuctor<fetch::vm::Ptr<fetch::vm::String>, fetch::vm::Ptr<fetch::vm::String>>()
      .CreateMemberFunction("GetData", &DataLoaderWrapper::GetData)
      .CreateMemberFunction("Display", &DataLoaderWrapper::Display);

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
