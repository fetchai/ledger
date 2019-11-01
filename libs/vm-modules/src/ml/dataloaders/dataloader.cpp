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
#include "math/utilities/ReadCSV.hpp"
#include "ml/dataloaders/commodity_dataloader.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "vm/array.hpp"
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

VMDataLoader::VMDataLoader(VM *vm, TypeId type_id, Ptr<String> const &mode)
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

Ptr<VMDataLoader> VMDataLoader::Constructor(VM *vm, TypeId type_id, Ptr<String> const &mode)
{
  return Ptr<VMDataLoader>{new VMDataLoader(vm, type_id, mode)};
}

void VMDataLoader::Bind(Module &module)
{
  module.CreateClassType<VMDataLoader>("DataLoader")
      .CreateConstructor(&VMDataLoader::Constructor)
      .CreateSerializeDefaultConstructor([](VM *vm, TypeId type_id) -> Ptr<VMDataLoader> {
        return Ptr<VMDataLoader>{new VMDataLoader(vm, type_id)};
      })
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
    throw std::runtime_error("current dataloader mode does not support AddDataByFiles");
  }
  }
}

void VMDataLoader::AddDataByData(
    fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<VMTensorType>>> const &data,
    Ptr<VMTensorType> const &                                             labels)
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
    throw std::runtime_error("current dataloader mode does not support AddDataByData");
  }
  }
}

void VMDataLoader::AddCommodityData(Ptr<String> const &xfilename, Ptr<String> const &yfilename)
{
  auto data  = fetch::math::utilities::ReadCSV<MathTensorType>(xfilename->str);
  auto label = fetch::math::utilities::ReadCSV<MathTensorType>(yfilename->str);

  std::static_pointer_cast<CommodityLoaderType>(loader_)->AddData({data}, label);
}

void VMDataLoader::AddMnistData(Ptr<String> const &xfilename, Ptr<String> const &yfilename)
{
  std::static_pointer_cast<MnistLoaderType>(loader_)->SetupWithDataFiles(xfilename->str,
                                                                         yfilename->str);
}

void VMDataLoader::AddTensorData(
    fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<VMTensorType>>> const &data,
    Ptr<VMTensorType> const &                                             labels)
{
  auto                    n_elements = data->elements.size();
  std::vector<TensorType> c_data(n_elements);

  for (fetch::math::SizeType i{0}; i < n_elements; i++)
  {
    Ptr<VMTensorType> ptr_tensor = data->elements.at(i);
    c_data.at(i)                 = (ptr_tensor)->GetTensor();
  }

  std::static_pointer_cast<TensorLoaderType>(loader_)->AddData(c_data, labels->GetTensor());
}

// TODO(issue 1692): Simplify Array<Tensor> construction
Ptr<VMTrainingPair> VMDataLoader::GetNext()
{
  std::pair<fetch::math::Tensor<DataType>, std::vector<fetch::math::Tensor<DataType>>> next =
      loader_->GetNext();

  auto first  = this->vm_->CreateNewObject<math::VMTensor>(next.first);
  auto second = this->vm_->CreateNewObject<math::VMTensor>(next.second.at(0));

  auto second_vector =
      this->vm_->CreateNewObject<fetch::vm::Array<Ptr<fetch::vm_modules::math::VMTensor>>>(
          second->GetTypeId(), static_cast<int32_t>(next.second.size()));

  TemplateParameter1 first_element(second, second->GetTypeId());

  AnyInteger first_index(0, TypeIds::UInt16);
  second_vector->SetIndexedValue(first_index, first_element);

  for (fetch::math::SizeType i{1}; i < next.second.size(); i++)
  {
    second = this->vm_->CreateNewObject<math::VMTensor>(next.second.at(i));
    TemplateParameter1 element(second, second->GetTypeId());

    AnyInteger index = AnyInteger(i, TypeIds::UInt16);
    second_vector->SetIndexedValue(index, element);
  }

  auto dataHolder = this->vm_->CreateNewObject<VMTrainingPair>(first, second_vector);

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
