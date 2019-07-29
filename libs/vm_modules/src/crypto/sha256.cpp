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

#include "crypto/sha256.hpp"
#include "vm/module.hpp"
#include "vm/object.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm_modules/crypto/sha256.hpp"
#include "vm_modules/math/bignumber.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {

void SHA256Wrapper::Bind(Module &module)
{
  module.CreateClassType<SHA256Wrapper>("SHA256")
      .CreateConstructor<>()
      .CreateMemberFunction("update", &SHA256Wrapper::UpdateUInt256)
      .CreateMemberFunction("update", &SHA256Wrapper::UpdateString)
      .CreateMemberFunction("update", &SHA256Wrapper::UpdateBuffer)
      .CreateMemberFunction("final", &SHA256Wrapper::Final)
      .CreateMemberFunction("reset", &SHA256Wrapper::Reset);
}

void SHA256Wrapper::UpdateUInt256(Ptr<math::UInt256Wrapper> const &uint)
{
  hasher_.Update(uint->number().pointer(), uint->number().size());
}

void SHA256Wrapper::UpdateString(Ptr<String> const &str)
{
  hasher_.Update(str->str);
}

void SHA256Wrapper::UpdateBuffer(Ptr<ByteArrayWrapper> const &buffer)
{
  hasher_.Update(buffer->byte_array());
}

void SHA256Wrapper::Reset()
{
  hasher_.Reset();
}

Ptr<math::UInt256Wrapper> SHA256Wrapper::Final()
{
  return vm_->CreateNewObject<math::UInt256Wrapper>(hasher_.Final());
}

SHA256Wrapper::SHA256Wrapper(VM *vm, TypeId type_id)
  : Object(vm, type_id)
{}

Ptr<SHA256Wrapper> SHA256Wrapper::Constructor(VM *vm, TypeId type_id)
{
  return new SHA256Wrapper(vm, type_id);
}

}  // namespace vm_modules
}  // namespace fetch
