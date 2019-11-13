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

#include "fake_learner_networker.hpp"
#include "vm_modules/dmlf/update.hpp"

#include <memory>

namespace fetch {

namespace vm {
class Module;
class Address;
}  // namespace vm

namespace vm_modules {
namespace dmlf {

class VMCoLearner : public fetch::vm::Object
{
public:
  using VMUpdateType  = VMUpdate;
  using CPPUpdateType = VMUpdateType::CPPType;
  using CPPType       = fetch::dmlf::FakeLearner<CPPUpdateType>;
  using CPPTypePtr    = std::unique_ptr<CPPType>;

  VMCoLearner(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  VMCoLearner(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::string id);

  static fetch::vm::Ptr<VMCoLearner> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static fetch::vm::Ptr<VMCoLearner> ConstructorFromId(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::vm::Ptr<fetch::vm::Address> const &addr);

  void SetId(fetch::vm::Ptr<fetch::vm::Address> const &addr);

  fetch::vm::Ptr<fetch::vm::Address> GetId();

  void PushUpdate(fetch::vm::Ptr<VMUpdateType> const &update);

  fetch::vm::Ptr<VMUpdateType> GetUpdate();

  uint64_t GetUpdateCount();

  static void Bind(fetch::vm::Module &module);

  CPPType &GetLearner();

private:
  CPPTypePtr  learner_;
  std::string id_;
};

}  // namespace dmlf
}  // namespace vm_modules
}  // namespace fetch
