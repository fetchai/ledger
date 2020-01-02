//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "ml/dataloaders/dataloader.hpp"

#include "core/serializers/main_serializer.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "vm/array.hpp"
#include "vm/module.hpp"
#include "vm/pair.hpp"
#include "vm_modules/ml/dataloaders/dataloader.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {

using MathTensorType        = fetch::math::Tensor<VMDataLoader::DataType>;
using VMTensorType          = fetch::vm_modules::math::VMTensor;
using VMTensorPtrType       = vm::Ptr<VMTensorType>;
using VMTensorArrayType     = fetch::vm::Array<VMTensorPtrType>;
using VMTensorArrayPtrType  = vm::Ptr<VMTensorArrayType>;
using VMTrainingPairType    = vm::Pair<VMTensorPtrType, VMTensorArrayPtrType>;
using VMTrainingPairPtrType = vm::Ptr<VMTrainingPairType>;

using TensorLoaderType =
    fetch::ml::dataloaders::TensorDataLoader<fetch::math::Tensor<VMDataLoader::DataType>,
                                             fetch::math::Tensor<VMDataLoader::DataType>>;

VMDataLoader::VMDataLoader(VM *vm, TypeId type_id)
  : Object(vm, type_id)
{}

VMDataLoader::VMDataLoader(VM *vm, TypeId type_id, Ptr<String> const &mode)
  : Object(vm, type_id)
{
  if (mode->string() == "tensor")
  {
    mode_   = DataLoaderMode::TENSOR;
    loader_ = std::make_shared<TensorLoaderType>();
  }
  else
  {
    throw std::runtime_error("Unknown dataloader mode : " + mode->string());
  }
}

Ptr<VMDataLoader> VMDataLoader::Constructor(VM *vm, TypeId type_id, Ptr<String> const &mode)
{
  try
  {
    return Ptr<VMDataLoader>{new VMDataLoader(vm, type_id, mode)};
  }
  catch (std::exception const &e)
  {
    vm->RuntimeError(e.what());
    return Ptr<VMDataLoader>{new VMDataLoader(vm, type_id)};
  }
}

void VMDataLoader::Bind(Module &module, bool const enable_experimental)
{
  if (enable_experimental)
  {
    module.CreateClassType<VMDataLoader>("DataLoader")
        .CreateConstructor(&VMDataLoader::Constructor, vm::MAXIMUM_CHARGE)
        .CreateSerializeDefaultConstructor([](VM *vm, TypeId type_id) -> Ptr<VMDataLoader> {
          return Ptr<VMDataLoader>{new VMDataLoader(vm, type_id)};
        })
        .CreateMemberFunction("addData", &VMDataLoader::AddDataByData, vm::MAXIMUM_CHARGE)
        .CreateMemberFunction("getNext", &VMDataLoader::GetNext, vm::MAXIMUM_CHARGE)
        .CreateMemberFunction("isDone", &VMDataLoader::IsDone, vm::MAXIMUM_CHARGE);
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
    RuntimeError("Current dataloader mode does not support AddDataByData");
    return;
  }
  }
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
VMTrainingPairPtrType VMDataLoader::GetNext()
{

  // Get pair from loader
  auto next = loader_->GetNext();

  // Create VMPair
  auto dataHolder = this->vm_->CreateNewObject<VMTrainingPairType>();

  // Create and set VMPair.first
  auto first = this->vm_->CreateNewObject<VMTensorType>(next.first);
  // Convert VMTensor to TemplateParameter1
  TemplateParameter1 t_first(first, first->GetTypeId());
  dataHolder->SetFirst(t_first);

  // Create and set VMPair.second
  auto second = this->vm_->CreateNewObject<VMTensorType>(next.second.at(0));

  // Add all elements to VMArray<VMTensor>
  auto second_vector = this->vm_->CreateNewObject<VMTensorArrayType>(
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

  // Convert VMArray<VMTensor> to TemplateParameter2
  TemplateParameter2 t_second(second_vector, second_vector->GetTypeId());
  dataHolder->SetSecond(t_second);

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
