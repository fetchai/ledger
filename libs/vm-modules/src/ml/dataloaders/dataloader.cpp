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

#include "core/serialisers/main_serialiser.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "vm/array.hpp"
#include "vm/module.hpp"
#include "vm/pair.hpp"
#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_modules/ml/utilities/conversion_utilities.hpp"
#include "vm_modules/use_estimator.hpp"

#include <memory>
#include <stdexcept>

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
    fetch::ml::dataloaders::TensorDataLoader<fetch::math::Tensor<VMDataLoader::DataType>>;

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
    // dataloader construction always requires initialising some strings, ptrs etc. but is very
    // cheap
    static const ChargeAmount FIXED_CONSTRUCTION_CHARGE{100};

    module.CreateClassType<VMDataLoader>("DataLoader")
        .CreateConstructor(&VMDataLoader::Constructor, FIXED_CONSTRUCTION_CHARGE)
        .CreateSerializeDefaultConstructor([](VM *vm, TypeId type_id) -> Ptr<VMDataLoader> {
          return Ptr<VMDataLoader>{new VMDataLoader(vm, type_id)};
        })
        .CreateMemberFunction("addData", &VMDataLoader::AddDataByData,
                              UseMemberEstimator(&VMDataLoader::EstimateAddDataByData))
        .CreateMemberFunction("getNext", &VMDataLoader::GetNext,
                              UseMemberEstimator(&VMDataLoader::EstimateGetNext))
        .CreateMemberFunction("isDone", &VMDataLoader::IsDone,
                              UseMemberEstimator(&VMDataLoader::EstimateIsDone));
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
    auto                    n_elements = data->elements.size();
    std::vector<TensorType> c_data(n_elements);

    for (fetch::math::SizeType i{0}; i < n_elements; i++)
    {
      Ptr<VMTensorType> ptr_tensor = data->elements.at(i);
      c_data.at(i)                 = (ptr_tensor)->GetTensor();
    }

    std::static_pointer_cast<TensorLoaderType>(loader_)->AddData(c_data, labels->GetTensor());
    break;
  }
  case DataLoaderMode::NONE:
  default:
  {
    RuntimeError("Current dataloader mode does not support AddDataByData");
    return;
  }
  }
}

ChargeAmount VMDataLoader::EstimateAddDataByData(
    fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<VMTensorType>>> const &data,
    Ptr<VMTensorType> const &                                             labels)
{
  ChargeAmount cost;
  switch (mode_)
  {
  case DataLoaderMode::TENSOR:
  {
    auto                    n_elements = data->elements.size();
    std::vector<TensorType> c_data(n_elements);

    for (fetch::math::SizeType i{0}; i < n_elements; i++)
    {
      Ptr<VMTensorType> ptr_tensor = data->elements.at(i);
      c_data.at(i)                 = (ptr_tensor)->GetTensor();
    }

    cost = std::static_pointer_cast<TensorLoaderType>(loader_)->ChargeAddData(c_data,
                                                                              labels->GetTensor());
    break;
  }
  case DataLoaderMode::NONE:
  default:
  {
    cost = MAXIMUM_CHARGE;
  }
  }
  return cost;
}

VMTrainingPairPtrType VMDataLoader::GetNext()
{
  // Get pair from loader
  auto next = loader_->GetNext();

  // Create VMPair
  auto dataHolder = this->vm_->CreateNewObject<VMTrainingPairType>();

  // Create and set VMPair.first
  auto first = fetch::vm_modules::ml::utilities::VMTensorConverter(this->vm_, next.first);
  // Convert VMTensor to TemplateParameter1
  TemplateParameter1 t_first(first, first->GetTypeId());
  dataHolder->SetFirst(t_first);

  // Create and set VMPair.second
  auto second = fetch::vm_modules::ml::utilities::VMArrayConverter(this->vm_, next.second);

  // Convert VMArray<VMTensor> to TemplateParameter2
  TemplateParameter2 t_second(second, second->GetTypeId());
  dataHolder->SetSecond(t_second);

  return dataHolder;
}

ChargeAmount VMDataLoader::EstimateGetNext()
{
  ChargeAmount const cost = std::static_pointer_cast<TensorLoaderType>(loader_)->ChargeGetNext();
  return cost;
}

bool VMDataLoader::IsDone()
{
  return loader_->IsDone();
}

ChargeAmount VMDataLoader::EstimateIsDone()
{
  ChargeAmount const cost = std::static_pointer_cast<TensorLoaderType>(loader_)->ChargeIsDone();
  return cost;
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
