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

#include "crypto/sha256.hpp"
#include "vm/module.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm_modules/math/bignumber.hpp"

namespace fetch {
namespace vm_modules {

class SHA256Wrapper : public fetch::vm::Object
{
public:
  using ElementType = uint8_t;

  template <typename T>
  using Ptr    = fetch::vm::Ptr<T>;
  using String = fetch::vm::String;

  SHA256Wrapper()           = delete;
  ~SHA256Wrapper() override = default;

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<SHA256Wrapper>("SHA256")
        .CreateConstructor(&SHA256Wrapper::Constructor)
        .CreateMemberFunction("update", &SHA256Wrapper::UpdateUInt256)
        .CreateMemberFunction("update", &SHA256Wrapper::UpdateString)
        .CreateMemberFunction("update", &SHA256Wrapper::UpdateBuffer)
        .CreateMemberFunction("final", &SHA256Wrapper::Final)
        .CreateMemberFunction("reset", &SHA256Wrapper::Reset);
  }

  void UpdateUInt256(vm::Ptr<vm_modules::math::UInt256Wrapper> const &uint)
  {
    hasher_.Update(uint->number().pointer(), uint->number().size());
  }

  void UpdateString(vm::Ptr<vm::String> const &str)
  {
    hasher_.Update(str->str);
  }

  void UpdateBuffer(Ptr<ByteArrayWrapper> const &buffer)
  {
    hasher_.Update(buffer->byte_array());
  }

  void Reset()
  {
    hasher_.Reset();
  }

  Ptr<math::UInt256Wrapper> Final()
  {
    return vm_->CreateNewObject<math::UInt256Wrapper>(hasher_.Final());
  }

  Ptr<ByteArrayWrapper> FinalAsBuffer()
  {
    return vm_->CreateNewObject<ByteArrayWrapper>(hasher_.Final());
  }

  SHA256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
    : fetch::vm::Object(vm, type_id)
  {}

  static fetch::vm::Ptr<SHA256Wrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  {
    return new SHA256Wrapper(vm, type_id);
  }

private:
  crypto::SHA256 hasher_;
};

}  // namespace vm_modules
}  // namespace fetch
