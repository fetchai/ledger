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


#include "ml/utilities/scaler.hpp"
#include "ml/utilities/min_max_scaler.hpp"

#include "vm/module.hpp"

namespace fetch {
namespace vm_modules {
namespace ml {
namespace utilities {

class VMScaler : public fetch::vm::Object
{
  using DataType       = fetch::fixed_point::fp64_t;
  using MathTensorType = fetch::math::Tensor<DataType>;
  using VMTensorType   = fetch::vm_modules::math::VMTensor;

public:
  VMScaler(fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::vm::Ptr<fetch::vm::String> const &mode)
    : fetch::vm::Object(vm, type_id)
  {
    if (mode->str == "min_max")
    {
      scaler_ = std::make_shared<fetch::ml::utilities::MinMaxScaler<MathTensorType>>();
    }
    else
    {
      throw std::runtime_error("unrecognised mode type");
    }
  }

  static fetch::vm::Ptr<VMScaler> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::vm::Ptr<fetch::vm::String> const &mode)
  {
    return new VMScaler(vm, type_id, mode);
  }

  void SetScale(fetch::vm::Ptr<VMTensorType> const &reference_tensor)
  {
    scaler_.SetScaler(reference_tensor);
  }

  void Normalise(fetch::vm::Ptr<VMTensorType> const &input_tensor, fetch::vm::Ptr<VMTensorType> const &output_tensor)
  {
    scaler_.Normalise(input_tensor, output_tensor);
  }

  void DeNormalise(fetch::vm::Ptr<VMTensorType> const &input_tensor, fetch::vm::Ptr<VMTensorType> const &output_tensor)
  {
    scaler_.DeNormalise(input_tensor, output_tensor);
  }

  static void Bind(fetch::vm::Module &module)
  {
    module.CreateClassType<VMScaler>("Graph")
        .CreateConstuctor<fetch::vm::Ptr<fetch::vm::String> const &>()
        .CreateMemberFunction("setScale", &VMScaler::SetScale)
        .CreateMemberFunction("normalise", &VMScaler::Normalise)
        .CreateMemberFunction("deNormalise", &VMScaler::DeNormalise);
  }

  std::shared_ptr<fetch::ml::utilities::Scaler<fetch::math::Tensor<DataType>>> scaler_;
};

}  // namespace utilities
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
