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
#include "ml/dataloaders/mnist_loader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/mean_square_error.hpp"

#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include "vm_modules/ml/cross_entropy.hpp"
#include "vm_modules/ml/graph.hpp"

#include <fstream>
#include <sstream>
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

class TrainingPairWrapper
  : public fetch::vm::Object,
    public std::pair<uint64_t, fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper>>
{
public:
  TrainingPairWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                      fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> t)
    : fetch::vm::Object(vm, type_id)
  {
    this->second = t;
  }

  static fetch::vm::Ptr<TrainingPairWrapper> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> t)
  {
    return new TrainingPairWrapper(vm, type_id, t);
  }

  fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> data()
  {
    return this->second;
  }

  uint64_t label()
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
    std::pair<uint64_t, std::shared_ptr<fetch::math::Tensor<float>>> d = loader_.GetNext();
    std::shared_ptr<fetch::math::Tensor<float>>                      a = *(dataHolder->second);
    a->Copy(*(d.second));
    dataHolder->first = d.first;
    return dataHolder;
  }

  void Display(fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> const &d)
  {
    loader_.Display(*d);
  }

private:
  fetch::ml::MNISTLoader<fetch::math::Tensor<float>> loader_;
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
    exit(-9);
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

  module->CreateFreeFunction("Print", &PrintNumber<int>);
  module->CreateFreeFunction("Print", &PrintNumber<uint64_t>);
  module->CreateFreeFunction("Print", &PrintNumber<float>);
  module->CreateFreeFunction("Print", &PrintNumber<double>);
  module->CreateFreeFunction("Print", &Print);
  module->CreateFreeFunction("toString", &toString);

  module->CreateClassType<System>("System")
      .CreateTypeFunction("Argc", &System::Argc)
      .CreateTypeFunction("Argv", &System::Argv);

  module->CreateTemplateInstantiationType<fetch::vm::Array, uint64_t>(fetch::vm::TypeIds::IArray);

  fetch::vm_modules::ml::CreateTensor(module);
  fetch::vm_modules::ml::CreateGraph(module);
  fetch::vm_modules::ml::CreateCrossEntropy(module);

  module->CreateClassType<TrainingPairWrapper>("TrainingPair")
      .CreateTypeConstuctor<fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper>>()
      .CreateInstanceFunction("Data", &TrainingPairWrapper::data)
      .CreateInstanceFunction("Label", &TrainingPairWrapper::label);

  module->CreateClassType<DataLoaderWrapper>("MNISTLoader")
      .CreateTypeConstuctor<fetch::vm::Ptr<fetch::vm::String>, fetch::vm::Ptr<fetch::vm::String>>()
      .CreateInstanceFunction("GetData", &DataLoaderWrapper::GetData)
      .CreateInstanceFunction("Display", &DataLoaderWrapper::Display);

  // Setting compiler up
  fetch::vm::Compiler *    compiler = new fetch::vm::Compiler(module.get());
  fetch::vm::Script        script;
  std::vector<std::string> errors;

  // Compiling
  bool compiled = compiler->Compile(source, "myscript", script, errors);

  if (!compiled)
  {
    std::cout << "Failed to compile" << std::endl;
    for (auto &s : errors)
    {
      std::cout << s << std::endl;
    }
    return -1;
  }

  if (!script.FindFunction("main"))
  {
    std::cout << "Function 'main' not found" << std::endl;
    return -2;
  }

  // Setting VM up and running
  std::string        error;
  fetch::vm::Variant output;

  fetch::vm::VM vm(module.get());
  if (!vm.Execute(script, "main", error, output))
  {
    std::cout << "Runtime error on line " << error << std::endl;
  }
  delete compiler;
  return 0;
}
