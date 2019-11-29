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

VMTensor::VMTensor(VM *vm, TypeId type_id, std::vector<uint64_t> const &shape)
  : Object(vm, type_id)
  , tensor_(shape)
{}

VMTensor::VMTensor(VM *vm, TypeId type_id, ArrayType tensor)
  : Object(vm, type_id)
  , tensor_(std::move(tensor))
{}

VMTensor::VMTensor(VM *vm, TypeId type_id)
  : Object(vm, type_id)
{}

Ptr<VMTensor> VMTensor::Constructor(VM *vm, TypeId type_id, Ptr<Array<SizeType>> const &shape)
{
  return Ptr<VMTensor>{new VMTensor(vm, type_id, shape->elements)};
}

void VMTensor::Bind(Module &module)
{
  using Index = fetch::math::SizeType;
  module.CreateClassType<VMTensor>("Tensor")
      .CreateConstructor(&VMTensor::Constructor)
      .CreateSerializeDefaultConstructor([](VM *vm, TypeId type_id) -> Ptr<VMTensor> {
        return Ptr<VMTensor>{new VMTensor(vm, type_id)};
      })
      .CreateMemberFunction("at", &VMTensor::At<Index>)
      .CreateMemberFunction("at", &VMTensor::At<Index, Index>)
      .CreateMemberFunction("at", &VMTensor::At<Index, Index, Index>)
      .CreateMemberFunction("at", &VMTensor::At<Index, Index, Index, Index>)
      .CreateMemberFunction("at", &VMTensor::At<Index, Index, Index, Index, Index>)
      .CreateMemberFunction("at", &VMTensor::At<Index, Index, Index, Index, Index, Index>)
      .CreateMemberFunction("setAt", &VMTensor::SetAt<Index, DataType>)
      .CreateMemberFunction("setAt", &VMTensor::SetAt<Index, Index, DataType>)
      .CreateMemberFunction("setAt", &VMTensor::SetAt<Index, Index, Index, DataType>)
      .CreateMemberFunction("setAt", &VMTensor::SetAt<Index, Index, Index, Index, DataType>)
      .CreateMemberFunction("setAt", &VMTensor::SetAt<Index, Index, Index, Index, Index, DataType>)
      .CreateMemberFunction("setAt",
                            &VMTensor::SetAt<Index, Index, Index, Index, Index, Index, DataType>)
      .CreateMemberFunction("fill", &VMTensor::Fill)
      .CreateMemberFunction("fillRandom", &VMTensor::FillRandom)
      .CreateMemberFunction("reshape", &VMTensor::Reshape)
      .CreateMemberFunction("squeeze", &VMTensor::Squeeze)
      .CreateMemberFunction("size", &VMTensor::size)
      .CreateMemberFunction("transpose", &VMTensor::Transpose)
      .CreateMemberFunction("unsqueeze", &VMTensor::Unsqueeze)
      .CreateMemberFunction("fromString", &VMTensor::FromString)
      .CreateMemberFunction("toString", &VMTensor::ToString);

  // Add support for Array of Tensors
  module.GetClassInterface<IArray>().CreateInstantiationType<Array<Ptr<VMTensor>>>();
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

template <typename... Indices>
VMTensor::DataType VMTensor::At(Indices... indices) const
{
  return tensor_.At(indices...);
}

template <typename... Args>
void VMTensor::SetAt(Args... args)
{
  tensor_.Set(args...);
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

Ptr<VMTensor> VMTensor::Unsqueeze()
{
  auto unsqueezed_tensor = tensor_.Copy();
  unsqueezed_tensor.Unsqueeze();
  return fetch::vm::Ptr<VMTensor>(new VMTensor(vm_, type_id_, unsqueezed_tensor));
}

bool VMTensor::Reshape(Ptr<Array<SizeType>> const &new_shape)
{
  return tensor_.Reshape(new_shape->elements);
}

void VMTensor::Transpose()
{
  tensor_.Transpose();
}

//////////////////////////////
/// PRINTING AND EXPORTING ///
//////////////////////////////

void VMTensor::FromString(fetch::vm::Ptr<fetch::vm::String> const &string)
{
  tensor_.Assign(fetch::math::Tensor<DataType>::FromString(string->string()));
}

Ptr<String> VMTensor::ToString() const
{
  return Ptr<String>{new String(vm_, tensor_.ToString())};
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
