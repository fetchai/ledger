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
{
  mode_ = DataLoaderMode::NONE;
}

VMDataLoader::VMDataLoader(VM *vm, TypeId type_id, Ptr<String> const &mode)
  : Object(vm, type_id)
{
  if (mode->str == "commodity")
  {
    mode_ = DataLoaderMode::COMMODITY;
  }
  else if (mode->str == "mnist")
  {
    mode_ = DataLoaderMode::MNIST;
  }
  else if (mode->str == "tensor")
  {
    mode_ = DataLoaderMode::TENSOR;
  }
  else
  {
    throw std::runtime_error("invalid dataloader mode");
  }
}

Ptr<VMDataLoader> VMDataLoader::Constructor(VM *vm, TypeId type_id, Ptr<String> const &mode)
{
  return new VMDataLoader(vm, type_id, mode);
}

void VMDataLoader::Bind(Module &module)
{
  module.CreateClassType<VMDataLoader>("DataLoader")
      .CreateConstructor(&VMDataLoader::Constructor)
      .CreateSerializeDefaultConstructor(
          [](VM *vm, TypeId type_id) { return new VMDataLoader(vm, type_id); })
      .CreateMemberFunction("addData", &VMDataLoader::AddDataByFiles)
      .CreateMemberFunction("addData", &VMDataLoader::AddDataByData)
      .CreateMemberFunction("getNext", &VMDataLoader::GetNext)
      .CreateMemberFunction("isDone", &VMDataLoader::IsDone);
}

void VMDataLoader::AddDataByFiles(Ptr<String> const &xfilename, Ptr<String> const &yfilename)
{
  switch (mode_)
  {
  case DataLoaderMode::COMMODITY:
  {
    AddCommodityData(xfilename, yfilename);
    break;
  }
  case DataLoaderMode::MNIST:
  {
    AddMnistData(xfilename, yfilename);
    break;
  }
  default:
  {
    throw std::runtime_error("invalid dataloader mode for xfilename, yfilename input to addData");
  }
  }
}

void VMDataLoader::AddDataByData(Ptr<VMTensorType> const &data, Ptr<VMTensorType> const &labels)
{
  switch (mode_)
  {
  case DataLoaderMode::TENSOR:
  {
    AddTensorData(data, labels);
    break;
  }
  default:
  {
    throw std::runtime_error("invalid dataloader mode for tensor add data");
  }
  }
}

void VMDataLoader::AddCommodityData(Ptr<String> const &xfilename, Ptr<String> const &yfilename)
{
  CommodityLoaderType loader;
  loader.AddData(xfilename->str, yfilename->str);
  loader_ = std::make_shared<CommodityLoaderType>(loader);
}

void VMDataLoader::AddMnistData(Ptr<String> const &xfilename, Ptr<String> const &yfilename)
{
  loader_ = std::make_shared<MnistLoaderType>(xfilename->str, yfilename->str);
}

void VMDataLoader::AddTensorData(Ptr<VMTensorType> const &data, Ptr<VMTensorType> const &labels)
{
  TensorLoaderType loader(labels->GetTensor().shape(), {data->GetTensor().shape()});
  loader.AddData(data->GetTensor(), labels->GetTensor());
  loader_ = std::make_shared<TensorLoaderType>(loader);
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
