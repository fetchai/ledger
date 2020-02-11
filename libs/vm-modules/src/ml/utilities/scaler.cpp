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

#include "ml/utilities/scaler.hpp"

#include "ml/utilities/min_max_scaler.hpp"
#include "vm/module.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/ml/utilities/scaler.hpp"

#include <memory>
#include <stdexcept>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {
namespace utilities {

using MathTensorType   = fetch::math::Tensor<VMScaler::DataType>;
using VMTensorType     = fetch::vm_modules::math::VMTensor;
using ScalerType       = fetch::ml::utilities::Scaler<fetch::math::Tensor<VMScaler::DataType>>;
using MinMaxScalerType = fetch::ml::utilities::MinMaxScaler<MathTensorType>;

VMScaler::VMScaler(VM *vm, TypeId type_id)
  : Object(vm, type_id)
  , scaler_(std::make_shared<MinMaxScalerType>())
{}

Ptr<VMScaler> VMScaler::Constructor(VM *vm, TypeId type_id)
{
  return Ptr<VMScaler>{new VMScaler(vm, type_id)};
}

void VMScaler::SetScaleByData(Ptr<VMTensorType> const &reference_tensor, Ptr<String> const &mode)
{
  if (mode->string() == "min_max")
  {
    scaler_ = std::make_shared<MinMaxScalerType>();
  }
  else
  {
    RuntimeError("unrecognised mode type");
    return;
  }

  scaler_->SetScale(reference_tensor->GetConstTensor());
}

void VMScaler::SetScaleByRange(DataType const &min_val, DataType const &max_val)
{
  if (max_val < min_val)
  {
    RuntimeError("setScale using a numeric range [min, max] failed: max < min");
    return;
  }
  scaler_->SetScale(min_val, max_val);
}

Ptr<VMTensorType> VMScaler::Normalise(Ptr<VMTensorType> const &input_tensor)
{
  MathTensorType output_tensor(input_tensor->shape());
  scaler_->Normalise(input_tensor->GetConstTensor(), output_tensor);
  return this->vm_->CreateNewObject<VMTensorType>(output_tensor);
}

Ptr<VMTensorType> VMScaler::DeNormalise(Ptr<VMTensorType> const &input_tensor)
{
  MathTensorType output_tensor(input_tensor->shape());
  scaler_->DeNormalise(input_tensor->GetConstTensor(), output_tensor);
  return this->vm_->CreateNewObject<VMTensorType>(output_tensor);
}

void VMScaler::Bind(Module &module, bool const enable_experimental)
{
  if (enable_experimental)
  {
    module.CreateClassType<VMScaler>("Scaler")
        .CreateConstructor(VMScaler::Constructor, vm::MAXIMUM_CHARGE)
        .CreateMemberFunction("setScale", &VMScaler::SetScaleByData, vm::MAXIMUM_CHARGE)
        .CreateMemberFunction("setScale", &VMScaler::SetScaleByRange, vm::MAXIMUM_CHARGE)
        .CreateMemberFunction("normalise", &VMScaler::Normalise, vm::MAXIMUM_CHARGE)
        .CreateMemberFunction("deNormalise", &VMScaler::DeNormalise, vm::MAXIMUM_CHARGE);
  }
}

bool VMScaler::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  buffer << *this;
  return true;
}

bool VMScaler::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  auto scaler = std::make_shared<VMScaler>(this->vm_, this->type_id_);
  buffer >> *scaler;
  *this = *scaler;
  return true;
}

}  // namespace utilities
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
