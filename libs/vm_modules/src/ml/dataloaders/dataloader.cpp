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

Ptr<VMDataLoader> VMDataLoader::Constructor(VM *vm, TypeId type_id)
{
  return new VMDataLoader(vm, type_id);
}

void VMDataLoader::Bind(Module &module)
{
  module.CreateClassType<VMDataLoader>("DataLoader")
      .CreateConstructor(&VMDataLoader::Constructor)
      .CreateMemberFunction("addData", &VMDataLoader::AddDataByFiles)
      .CreateMemberFunction("addData", &VMDataLoader::AddDataByData)
      .CreateMemberFunction("getNext", &VMDataLoader::GetNext)
      .CreateMemberFunction("isDone", &VMDataLoader::IsDone);
}

void VMDataLoader::AddDataByFiles(Ptr<String> const &mode, Ptr<String> const &xfilename,
                                  Ptr<String> const &yfilename)
{
  if (mode->str == "commodity")
  {
    AddCommodityData(xfilename, yfilename);
  }
  else if (mode->str == "mnist")
  {
    AddMnistData(xfilename, yfilename);
  }
  else
  {
    throw std::runtime_error("mode not valid for xfilename, yfilename input to addData");
  }
}

void VMDataLoader::AddDataByData(Ptr<String> const &mode, Ptr<VMTensorType> const &data,
                                 Ptr<VMTensorType> const &labels)
{
  if (mode->str == "tensor")
  {
    AddTensorData(data, labels);
  }
  else
  {
    throw std::runtime_error("mode not valid for xfilename, yfilename input to addData");
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

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
