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
#include "ml/graph.hpp"
#include "ml/ops/fully_connected.hpp"
#include "ml/ops/relu.hpp"
#include "ml/ops/softmax.hpp"

#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include "mnist_loader.hpp"

#include <fstream>
#include <vector>
#include <sstream>

class TensorWrapper : public fetch::vm::Object, public std::shared_ptr<fetch::math::Tensor<float>>
{
public:
  TensorWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::vector<std::size_t> const &shape)
    : fetch::vm::Object(vm, type_id)
    , std::shared_ptr<fetch::math::Tensor<float>>(new fetch::math::Tensor<float>(shape))
  {}

  static fetch::vm::Ptr<TensorWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::vm::Ptr<fetch::vm::Array<uint64_t>> shape)
  {
    /*
    if(shape.type != TYPE_INT32)
      {
	vm->RuntimeError("constructor args must be array of ints");
	return nullptr;
      }
    */
    return new TensorWrapper(vm, type_id, shape->elements);
  }
};

class GraphWrapper : public fetch::vm::Object, public fetch::ml::Graph<fetch::math::Tensor<float>>
{
public:
  GraphWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
    :fetch::vm::Object(vm, type_id), fetch::ml::Graph<fetch::math::Tensor<float>>()
  {}

  static fetch::vm::Ptr<GraphWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  {
    return new GraphWrapper(vm, type_id);
  }

  void PassArray(fetch::vm::Ptr<fetch::vm::IArray> const &shape)
  {
    
  }

  void SetInput(fetch::vm::Ptr<fetch::vm::String> const &name, fetch::vm::Ptr<TensorWrapper> const &input)
  {
    fetch::ml::Graph<fetch::math::Tensor<float>>::SetInput(name->str, *input);
  }

  fetch::vm::Ptr<TensorWrapper> Evaluate(fetch::vm::Ptr<fetch::vm::String> const &name)
  {
    std::shared_ptr<fetch::math::Tensor<float>> t = fetch::ml::Graph<fetch::math::Tensor<float>>::Evaluate(name->str);
    // Need to convert std::shared_ptr to vm::Ptr
    return nullptr;
  }

  void AddPlaceholder(fetch::vm::Ptr<fetch::vm::String> const &name)
  {
    fetch::ml::Graph<fetch::math::Tensor<float>>::AddNode<fetch::ml::ops::PlaceHolder<fetch::math::Tensor<float>>>(name->str, {});
  }

  void AddFullyConnected(fetch::vm::Ptr<fetch::vm::String> const &name, fetch::vm::Ptr<fetch::vm::String> const &inputName, int in, int out)
  {
    fetch::ml::Graph<fetch::math::Tensor<float>>::AddNode<fetch::ml::ops::FullyConnected<fetch::math::Tensor<float>>>(name->str, {inputName->str}, std::size_t(in), std::size_t(out));
  }

  void AddRelu(fetch::vm::Ptr<fetch::vm::String> const &name, fetch::vm::Ptr<fetch::vm::String> const &inputName)
  {
    fetch::ml::Graph<fetch::math::Tensor<float>>::AddNode<fetch::ml::ops::ReluLayer<fetch::math::Tensor<float>>>(name->str, {inputName->str});
  }

  void AddSoftmax(fetch::vm::Ptr<fetch::vm::String> const &name, fetch::vm::Ptr<fetch::vm::String> const &inputName)
  {
    fetch::ml::Graph<fetch::math::Tensor<float>>::AddNode<fetch::ml::ops::SoftmaxLayer<fetch::math::Tensor<float>>>(name->str, {inputName->str});
  }
  
};

class TrainingPairWrapper : public fetch::vm::Object, public std::pair<uint64_t, fetch::vm::Ptr<TensorWrapper>>
{
public:
  TrainingPairWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::vm::Ptr<TensorWrapper> t)
    : fetch::vm::Object(vm, type_id)
  {
    this->second = t;
  }

  static fetch::vm::Ptr<TrainingPairWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::vm::Ptr<TensorWrapper> t)
  {
    return new TrainingPairWrapper(vm, type_id, t);
  }

  fetch::vm::Ptr<TensorWrapper> data()
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
  DataLoaderWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
    :fetch::vm::Object(vm, type_id)
  {}

  static fetch::vm::Ptr<DataLoaderWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  {
    return new DataLoaderWrapper(vm, type_id);
  }

  // Wont compile if parameter is not const &
  // The actual fetch::vm::Ptr is const, but the pointed to memory is modified
  fetch::vm::Ptr<TrainingPairWrapper> GetData(fetch::vm::Ptr<TrainingPairWrapper> const &dataHolder)
  {
    std::pair<unsigned int, std::shared_ptr<fetch::math::Tensor<float>>> d = loader_.GetNext(nullptr);
    std::shared_ptr<fetch::math::Tensor<float>> a = *(dataHolder->second);
    a->Copy(*(d.second));
    dataHolder->first = d.first;
    return dataHolder;
  }

private:
  MNISTLoader loader_;
};

template<typename T>
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

  // Reading file
  std::ifstream      file(argv[1], std::ios::binary);
  std::ostringstream ss;
  ss << file.rdbuf();
  const std::string source = ss.str();
  file.close();

  fetch::vm::Module module;

  module.CreateFreeFunction("Print", &Print);
  module.CreateFreeFunction("Print", &PrintNumber<int>);
  module.CreateFreeFunction("Print", &PrintNumber<uint64_t>);
  module.CreateFreeFunction("Print", &PrintNumber<float>);
  module.CreateFreeFunction("Print", &PrintNumber<double>);
  module.CreateFreeFunction("Print", &Print);
  module.CreateFreeFunction("Print", &Print);
  module.CreateFreeFunction("toString", &toString);

  module.CreateTemplateInstantiationType<fetch::vm::Array, uint64_t>(fetch::vm::TypeIds::IArray);
  
  module.CreateClassType<TensorWrapper>("Tensor")
    .CreateTypeConstuctor<fetch::vm::Ptr<fetch::vm::Array<uint64_t>>>();
  
  module.CreateClassType<GraphWrapper>("Graph")
    .CreateTypeConstuctor<>()
    .CreateInstanceFunction("PassArray", &GraphWrapper::PassArray)
    .CreateInstanceFunction("SetInput", &GraphWrapper::SetInput)
    .CreateInstanceFunction("Evaluate", &GraphWrapper::Evaluate)
    .CreateInstanceFunction("AddPlaceholder", &GraphWrapper::AddPlaceholder)
    .CreateInstanceFunction("AddFullyConnected", &GraphWrapper::AddFullyConnected)
    .CreateInstanceFunction("AddRelu", &GraphWrapper::AddRelu)
    .CreateInstanceFunction("AddSoftmax", &GraphWrapper::AddSoftmax);

  module.CreateClassType<TrainingPairWrapper>("TrainingPair")
    .CreateTypeConstuctor<fetch::vm::Ptr<TensorWrapper>>()
    .CreateInstanceFunction("Data", &TrainingPairWrapper::data)
    .CreateInstanceFunction("Label", &TrainingPairWrapper::label);

  module.CreateClassType<DataLoaderWrapper>("MNISTLoader")
    .CreateTypeConstuctor<>()
    .CreateInstanceFunction("GetData", &DataLoaderWrapper::GetData);
  
  // Setting compiler up
  fetch::vm::Compiler *    compiler = new fetch::vm::Compiler(&module);
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

  fetch::vm::VM vm(&module);
  if (!vm.Execute(script, "main", error, output))
  {
    std::cout << "Runtime error on line " << error << std::endl;
  }
  delete compiler;
  return 0;
}
