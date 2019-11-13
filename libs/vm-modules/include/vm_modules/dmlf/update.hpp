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

#include "dmlf/update.hpp"
#include "vm/array.hpp"
#include "vm_modules/math/tensor.hpp"

#include <memory>

namespace fetch {

namespace vm {
class Module;
}

namespace vm_modules {
namespace dmlf {

class VMUpdate : public fetch::vm::Object
{
public:
  using VMPayloadType  = math::VMTensor;
  using CPPPayloadType = VMPayloadType::TensorType;
  using CPPType        = fetch::dmlf::Update<CPPPayloadType>;
  using CPPTypePtr     = std::unique_ptr<CPPType>;

  VMUpdate(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  VMUpdate(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
           std::vector<CPPPayloadType> const payloads);

  static fetch::vm::Ptr<VMUpdate> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static fetch::vm::Ptr<VMUpdate> ConstructorFromVecPayload(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<VMPayloadType>>> const &payloads);

  void SetSource(fetch::vm::Ptr<fetch::vm::Address> const &addr);

  fetch::vm::Ptr<fetch::vm::Address> GetSource();

  fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<VMPayloadType>>> GetGradients();

  uint64_t GetTimestamp();

  static void Bind(fetch::vm::Module &module);

  CPPType &GetUpdate();

  void SetUpdate(CPPType const &from);

  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

private:
  CPPTypePtr update_;
};

}  // namespace dmlf
}  // namespace vm_modules
}  // namespace fetch
