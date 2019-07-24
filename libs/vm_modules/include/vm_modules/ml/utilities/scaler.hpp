#pragma once
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

#include "ml/utilities/min_max_scaler.hpp"
#include "ml/utilities/scaler.hpp"

#include "vm/module.hpp"

namespace fetch {
namespace vm_modules {
namespace ml {
namespace utilities {

class VMScaler : public fetch::vm::Object
{
  using DataType       = fetch::vm_modules::math::DataType;
  using MathTensorType = fetch::math::Tensor<DataType>;
  using VMTensorType   = fetch::vm_modules::math::VMTensor;

  using ScalerType       = fetch::ml::utilities::Scaler<fetch::math::Tensor<DataType>>;
  using MinMaxScalerType = fetch::ml::utilities::MinMaxScaler<MathTensorType>;

public:
  VMScaler(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
    : fetch::vm::Object(vm, type_id)
  {}

  static fetch::vm::Ptr<VMScaler> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  {
    return new VMScaler(vm, type_id);
  }

  void SetScale(fetch::vm::Ptr<VMTensorType> const &     reference_tensor,
                fetch::vm::Ptr<fetch::vm::String> const &mode)
  {
    if (mode->str == "min_max")
    {
      scaler_ = std::make_shared<MinMaxScalerType>();
    }
    else
    {
      throw std::runtime_error("unrecognised mode type");
    }

    scaler_->SetScale(reference_tensor->GetConstTensor());
  }

  fetch::vm::Ptr<VMTensorType> Normalise(fetch::vm::Ptr<VMTensorType> const &input_tensor)
  {
    MathTensorType output_tensor(input_tensor->shape());
    scaler_->Normalise(input_tensor->GetConstTensor(), output_tensor);
    return this->vm_->CreateNewObject<VMTensorType>(output_tensor);
  }

  fetch::vm::Ptr<VMTensorType> DeNormalise(fetch::vm::Ptr<VMTensorType> const &input_tensor)
  {
    MathTensorType output_tensor(input_tensor->shape());
    scaler_->DeNormalise(input_tensor->GetConstTensor(), output_tensor);
    return this->vm_->CreateNewObject<VMTensorType>(output_tensor);
  }

  static void Bind(fetch::vm::Module &module)
  {
    module.CreateClassType<VMScaler>("Scaler")
        .CreateConstuctor()
        .CreateMemberFunction("setScale", &VMScaler::SetScale)
        .CreateMemberFunction("normalise", &VMScaler::Normalise)
        .CreateMemberFunction("deNormalise", &VMScaler::DeNormalise);
  }

  std::shared_ptr<ScalerType> scaler_;
};

}  // namespace utilities
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
