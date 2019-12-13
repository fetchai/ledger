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
#include "vm/array.hpp"
#include "vm/module.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/math/type.hpp"

#include <cstdint>
#include <vector>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace math {

using ArrayType  = fetch::math::Tensor<VMTensor::DataType>;
using SizeType   = ArrayType::SizeType;
using SizeVector = ArrayType::SizeVector;

VMTensor::VMTensor(VM *vm, TypeId type_id, std::vector<std::uint64_t> const &shape)
  : Object(vm, type_id)
  , tensor_(shape)
{}

VMTensor::VMTensor(VM *vm, TypeId type_id, ArrayType tensor)
  : Object(vm, type_id)
  , tensor_(std::move(tensor))
{}

VMTensor::VMTensor(VM *vm, TypeId type_id)
  : Object(vm, type_id)
  , tensor_{}
{}

Ptr<VMTensor> VMTensor::Constructor(VM *vm, TypeId type_id, Ptr<Array<SizeType>> const &shape)
{
  return new VMTensor(vm, type_id, shape->elements);
}

void VMTensor::Bind(Module &module)
{
  module.CreateClassType<VMTensor>("Tensor")
      .CreateConstructor(&VMTensor::Constructor)
      .CreateSerializeDefaultConstructor(
          [](VM *vm, TypeId type_id) -> Ptr<VMTensor> { return new VMTensor(vm, type_id); })
      .CreateMemberFunction("at", &VMTensor::AtOne)
      .CreateMemberFunction("at", &VMTensor::AtTwo)
      .CreateMemberFunction("at", &VMTensor::AtThree)
      .CreateMemberFunction("at", &VMTensor::AtFour)
      .CreateMemberFunction("setAt", &VMTensor::SetAtOne)
      .CreateMemberFunction("setAt", &VMTensor::SetAtTwo)
      .CreateMemberFunction("setAt", &VMTensor::SetAtThree)
      .CreateMemberFunction("setAt", &VMTensor::SetAtFour)
      .CreateMemberFunction("fill", &VMTensor::Fill)
      .CreateMemberFunction("fillRandom", &VMTensor::FillRandom)
      .CreateMemberFunction("reshape", &VMTensor::Reshape)
      .CreateMemberFunction("squeeze", &VMTensor::Squeeze)
      .CreateMemberFunction("size", &VMTensor::size)
      .CreateMemberFunction("fromString", &VMTensor::FromString)
      .CreateMemberFunction("toString", &VMTensor::ToString);
}

SizeVector VMTensor::shape() const
{
  return tensor_.shape();
}

SizeType VMTensor::size() const
{
  return tensor_.size();
}

////////////////////////////////////
/// ACCESSING AND SETTING VALUES ///
////////////////////////////////////

DataType VMTensor::AtOne(SizeType idx1) const
{
  return tensor_.At(idx1);
}

DataType VMTensor::AtTwo(uint64_t idx1, uint64_t idx2) const
{
  return tensor_.At(idx1, idx2);
}

DataType VMTensor::AtThree(uint64_t idx1, uint64_t idx2, uint64_t idx3) const
{
  return tensor_.At(idx1, idx2, idx3);
}

DataType VMTensor::AtFour(uint64_t idx1, uint64_t idx2, uint64_t idx3, uint64_t idx4) const
{
  return tensor_.At(idx1, idx2, idx3, idx4);
}

void VMTensor::SetAtOne(uint64_t idx1, DataType value)
{
  tensor_.At(idx1) = value;
}

void VMTensor::SetAtTwo(uint64_t idx1, uint64_t idx2, DataType value)
{
  tensor_.At(idx1, idx2) = value;
}

void VMTensor::SetAtThree(uint64_t idx1, uint64_t idx2, uint64_t idx3, DataType value)
{
  tensor_.At(idx1, idx2, idx3) = value;
}

void VMTensor::SetAtFour(uint64_t idx1, uint64_t idx2, uint64_t idx3, uint64_t idx4, DataType value)
{
  tensor_.At(idx1, idx2, idx3, idx4) = value;
}

void VMTensor::Copy(ArrayType const &other)
{
  tensor_.Copy(other);
}

void VMTensor::Fill(DataType const &value)
{
  tensor_.Fill(value);
}

void VMTensor::FillRandom()
{
  tensor_.FillUniformRandom();
}

Ptr<VMTensor> VMTensor::Squeeze()
{
  auto squeezed_tensor = tensor_.Copy();
  squeezed_tensor.Squeeze();
  return fetch::vm::Ptr<VMTensor>(new VMTensor(vm_, type_id_, squeezed_tensor));
}

bool VMTensor::Reshape(Ptr<Array<SizeType>> const &new_shape)
{
  return tensor_.Reshape(new_shape->elements);
}

//////////////////////////////
/// PRINTING AND EXPORTING ///
//////////////////////////////

Ptr<VMTensor> VMTensor::FromString(fetch::vm::Ptr<fetch::vm::String> const &string)
{
  auto string_tensor = tensor_.Copy();
  string_tensor.Assign(fetch::math::Tensor<DataType>::FromString(string->str));
  return fetch::vm::Ptr<VMTensor>(new VMTensor(vm_, type_id_, string_tensor));
}

Ptr<String> VMTensor::ToString() const
{
  return new String(vm_, tensor_.ToString());
}

ArrayType &VMTensor::GetTensor()
{
  return tensor_;
}

ArrayType const &VMTensor::GetConstTensor()
{
  return tensor_;
}

bool VMTensor::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  buffer << tensor_;
  return true;
}

bool VMTensor::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  buffer >> tensor_;
  return true;
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
