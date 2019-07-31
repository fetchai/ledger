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

#include "core/byte_array/byte_array.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"

#include <cstdint>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {

void ByteArrayWrapper::Bind(Module &module)
{
  module.CreateClassType<ByteArrayWrapper>("Buffer")
      .CreateConstructor<int32_t>()
      .CreateMemberFunction("copy", &ByteArrayWrapper::Copy);
}

ByteArrayWrapper::ByteArrayWrapper(VM *vm, TypeId type_id, byte_array::ByteArray bytearray)
  : Object(vm, type_id)
  , byte_array_(std::move(bytearray))
{}

Ptr<ByteArrayWrapper> ByteArrayWrapper::Constructor(VM *vm, TypeId type_id, int32_t n)
{
  return new ByteArrayWrapper(vm, type_id, byte_array::ByteArray(std::size_t(n)));
}

Ptr<ByteArrayWrapper> ByteArrayWrapper::Copy()
{
  return vm_->CreateNewObject<ByteArrayWrapper>(byte_array_.Copy());
}

byte_array::ByteArray ByteArrayWrapper::byte_array() const
{
  return byte_array_;
}

bool ByteArrayWrapper::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  buffer << byte_array_;
  return true;
}

bool ByteArrayWrapper::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  buffer >> byte_array_;
  return true;
}

}  // namespace vm_modules
}  // namespace fetch
