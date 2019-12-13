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

#include "core/serializers/main_serializer.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/commodity_dataloader.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "vm/module.hpp"
#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_modules/ml/training_pair.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {

using MathTensorType = fetch::math::Tensor<VMDataLoader::DataType>;
using VMTensorType   = fetch::vm_modules::math::VMTensor;

using CommodityLoaderType =
    fetch::ml::dataloaders::CommodityDataLoader<fetch::math::Tensor<VMDataLoader::DataType>,
                                                fetch::math::Tensor<VMDataLoader::DataType>>;
using MnistLoaderType =
    fetch::ml::dataloaders::MNISTLoader<fetch::math::Tensor<VMDataLoader::DataType>,
                                        fetch::math::Tensor<VMDataLoader::DataType>>;
using TensorLoaderType =
    fetch::ml::dataloaders::TensorDataLoader<fetch::math::Tensor<VMDataLoader::DataType>,
                                             fetch::math::Tensor<VMDataLoader::DataType>>;

VMDataLoader::VMDataLoader(VM *vm, TypeId type_id)
  : Object(vm, type_id)
{}

VMDataLoader::VMDataLoader(VM *vm, TypeId type_id, fetch::vm::Ptr<fetch::vm::String> const &mode)
  : Object(vm, type_id)
{
  if (mode->str == "tensor")
  {
    mode_   = DataLoaderMode::TENSOR;
    loader_ = std::make_shared<TensorLoaderType>();
  }
  else if (mode->str == "commodity")
  {
    mode_   = DataLoaderMode::COMMODITY;
    loader_ = std::make_shared<CommodityLoaderType>();
  }
  else if (mode->str == "mnist")
  {
    mode_   = DataLoaderMode::MNIST;
    loader_ = std::make_shared<MnistLoaderType>();
  }
  else
  {
    throw std::runtime_error("unknown dataloader mode");
  }
}

VMDataLoader::VMDataLoader(VM *vm, TypeId type_id, DataLoaderMode const &mode)
  : Object(vm, type_id)
{
  switch (mode)
  {
  case DataLoaderMode::TENSOR:
  {
    loader_ = std::make_shared<TensorLoaderType>();
  }
  case DataLoaderMode::COMMODITY:
  {
    loader_ = std::make_shared<CommodityLoaderType>();
  }
  case DataLoaderMode::MNIST:
  {
    loader_ = std::make_shared<MnistLoaderType>();
  }
  default:
  {
    throw std::runtime_error("unknown dataloader mode");
  }
  }
}

fetch::vm::Ptr<VMDataLoader> VMDataLoader::Constructor(
    fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::vm::Ptr<fetch::vm::String> const &mode)
{
  return new VMDataLoader(vm, type_id, mode);
}

void VMDataLoader::Bind(Module &module)
{
  module.CreateClassType<VMDataLoader>("DataLoader")
      .CreateConstructor(&VMDataLoader::Constructor)
      .CreateSerializeDefaultConstructor(
          [](VM *vm, TypeId type_id) -> Ptr<VMDataLoader> { return new VMDataLoader(vm, type_id); })
      .CreateMemberFunction("addData", &VMDataLoader::AddDataByFiles)
      .CreateMemberFunction("addData", &VMDataLoader::AddDataByData)
      .CreateMemberFunction("getNext", &VMDataLoader::GetNext)
      .CreateMemberFunction("isDone", &VMDataLoader::IsDone);
}

fetch::vm::Ptr<VMDataLoader> VMDataLoader::AddDataByFiles(Ptr<String> const &xfilename,
                                                          Ptr<String> const &yfilename)
{
  auto add_data_loader = fetch::vm::Ptr<VMDataLoader>(new VMDataLoader(vm_, type_id_, mode_));

  switch (mode_)
  {
  case DataLoaderMode::COMMODITY:
  {
    add_data_loader->AddCommodityData(xfilename, yfilename);
    break;
  }
  case DataLoaderMode::MNIST:
  {
    add_data_loader->AddMnistData(xfilename, yfilename);
    break;
  }
  default:
  {
    throw std::runtime_error("current dataloader mode does not support AddDataByFiles");
  }
  }

  return add_data_loader;
}

fetch::vm::Ptr<VMDataLoader> VMDataLoader::AddDataByData(Ptr<VMTensorType> const &data,
                                                         Ptr<VMTensorType> const &labels)
{
  auto add_data_loader = fetch::vm::Ptr<VMDataLoader>(new VMDataLoader(vm_, type_id_, mode_));

  switch (mode_)
  {
  case DataLoaderMode::TENSOR:
  {
    add_data_loader->AddTensorData(data, labels);
    break;
  }
  default:
  {
    throw std::runtime_error("current dataloader mode does not support AddDataByData");
  }
  }

  return add_data_loader;
}

void VMDataLoader::AddCommodityData(Ptr<String> const &xfilename, Ptr<String> const &yfilename)
{
  auto data  = fetch::ml::dataloaders::ReadCSV<MathTensorType>(xfilename->str);
  auto label = fetch::ml::dataloaders::ReadCSV<MathTensorType>(yfilename->str);

  std::static_pointer_cast<CommodityLoaderType>(loader_)->AddData(data, label);
}

void VMDataLoader::AddMnistData(Ptr<String> const &xfilename, Ptr<String> const &yfilename)
{
  std::static_pointer_cast<MnistLoaderType>(loader_)->SetupWithDataFiles(xfilename->str,
                                                                         yfilename->str);
}

void VMDataLoader::AddTensorData(Ptr<VMTensorType> const &data, Ptr<VMTensorType> const &labels)
{
  std::static_pointer_cast<TensorLoaderType>(loader_)->AddData(data->GetTensor(),
                                                               labels->GetTensor());
}

Ptr<VMTrainingPair> VMDataLoader::GetNext()
{
  std::pair<fetch::math::Tensor<DataType>, std::vector<fetch::math::Tensor<DataType>>> next =
      loader_->GetNext();

  auto first      = this->vm_->CreateNewObject<math::VMTensor>(next.first);
  auto second     = this->vm_->CreateNewObject<math::VMTensor>(next.second.at(0));
  auto dataHolder = this->vm_->CreateNewObject<VMTrainingPair>(first, second);

  return dataHolder;
}

bool VMDataLoader::IsDone()
{
  return loader_->IsDone();
}

VMDataLoader::DataLoaderPtrType &VMDataLoader::GetDataLoader()
{
  return loader_;
}

bool VMDataLoader::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  buffer << *this;
  return true;
}

bool VMDataLoader::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  buffer.seek(0);
  auto dl = std::make_shared<fetch::vm_modules::ml::VMDataLoader>(this->vm_, this->type_id_);
  buffer >> *dl;
  *this = *dl;
  return true;
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
