#pragma once
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

#include "math/base_types.hpp"
#include "vm/array.hpp"
#include "vm/string.hpp"
#include "vm/vm.hpp"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_modules/vm_factory.hpp"

namespace fetch {
namespace vm_modules {
namespace ml {
namespace utilities {

using SizeType      = fetch::math::SizeType;
using VMStringPtr   = fetch::vm::Ptr<fetch::vm::String>;
using ChargeAmount  = fetch::vm::ChargeAmount;
using VMTensor      = fetch::vm_modules::math::VMTensor;
using VMTensorPtr   = fetch::vm::Ptr<VMTensor>;
using VMTensorArray = fetch::vm::Array<VMTensorPtr>;

////////////////////////
// Helpers to construct VM objects from non-VM ones //
////////////////////////

VMStringPtr VMStringConverter(fetch::vm::VM *vm, std::string const &str);

VMTensorPtr VMTensorConverter(fetch::vm::VM *vm, std::vector<fetch::math::SizeType> const &shape);

template <class DataType>
VMTensorPtr VMTensorConverter(fetch::vm::VM *vm, fetch::math::Tensor<DataType> const &tensor)
{
  return vm->CreateNewObject<VMTensor>(tensor);
}

template <class DataType>
fetch::vm::Ptr<fetch::vm::Array<uint64_t>> VMArrayConverter(fetch::vm::VM *              vm,
                                                            std::vector<DataType> const &values)
{
  std::size_t                                size = values.size();
  fetch::vm::Ptr<fetch::vm::Array<DataType>> array =
      vm->CreateNewObject<fetch::vm::Array<DataType>>(vm->GetTypeId<DataType>(),
                                                      static_cast<int32_t>(size));

  for (std::size_t i{0}; i < size; ++i)
  {
    array->elements[i] = values[i];
  }

  return array;
}

template <class DataType>
fetch::vm::Ptr<VMTensorArray> VMArrayConverter(
    fetch::vm::VM *vm, std::vector<fetch::math::Tensor<DataType>> const &values)
{
  // Create value_0
  auto                          value_0 = VMTensorConverter(vm, values.at(0));
  fetch::vm::TemplateParameter1 first_element(value_0, value_0->GetTypeId());

  // Create an array of tensors
  fetch::vm::Ptr<VMTensorArray> vm_array;
  vm_array =
      vm->CreateNewObject<VMTensorArray>(value_0->GetTypeId(), static_cast<int32_t>(values.size()));

  // set first value
  fetch::vm::AnyInteger first_index(0, fetch::vm::TypeIds::UInt16);
  vm_array->SetIndexedValue(first_index, first_element);

  // set subsequent values
  for (fetch::math::SizeType i{1}; i < values.size(); i++)
  {
    value_0 = VMTensorConverter(vm, values.at(i));
    fetch::vm::TemplateParameter1 element(value_0, value_0->GetTypeId());

    fetch::vm::AnyInteger index = fetch::vm::AnyInteger(i, fetch::vm::TypeIds::UInt16);
    vm_array->SetIndexedValue(index, element);
  }

  return vm_array;
}

}  // namespace utilities
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
