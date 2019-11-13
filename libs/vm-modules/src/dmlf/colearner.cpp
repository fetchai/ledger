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

#include "vm/module.hpp"
#include "vm/address.hpp"

#include "vm_modules/dmlf/colearner.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace dmlf {

VMCoLearner::VMCoLearner(VM *vm, TypeId type_id)
  : Object(vm, type_id)
{
  learner_ = std::make_unique<CPPType>();
}

VMCoLearner::VMCoLearner(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::string id)
  : Object(vm, type_id)
{
  learner_ = std::make_unique<CPPType>();
  id_ = std::move(id);
}

Ptr<VMCoLearner> VMCoLearner::Constructor(VM *vm, TypeId type_id)
{
  return Ptr<VMCoLearner>{new VMCoLearner(vm, type_id)};
}
  
Ptr<VMCoLearner> VMCoLearner::ConstructorFromId(fetch::vm::VM *vm, fetch::vm::TypeId type_id, Ptr<Address> const& id)
{
  return Ptr<VMCoLearner>{new VMCoLearner(vm, type_id, id->AsString()->str)};
}

void VMCoLearner::SetId(Ptr<Address> const& addr)
{
  id_ = addr->AsString()->str;
}

Ptr<Address> VMCoLearner::GetId()
{
  Ptr<Address> source = vm_->CreateNewObject<fetch::vm::Address>(Ptr<String>{new String{vm_, id_}});
  return source;
}

uint64_t VMCoLearner::GetUpdateCount()
{
  return static_cast<uint64_t>(learner_->GetUpdateCount());
}

Ptr<VMCoLearner::VMUpdateType> VMCoLearner::GetUpdate()
{
  CPPType::UpdateTypePtr update_cpp = learner_->GetUpdate();
  Ptr<VMUpdateType> update = vm_->CreateNewObject<VMUpdateType>();
  update->SetUpdate(*update_cpp);
  return update;
}

void VMCoLearner::PushUpdate(fetch::vm::Ptr<VMUpdateType> const& update)
{
  learner_->PushUpdate(std::make_shared<CPPUpdateType>(update->GetUpdate()));
}

void VMCoLearner::Bind(Module &module)
{
  module.CreateClassType<VMCoLearner>("CollaborativeLearner")
      //.CreateConstructor(&VMCoLearner::Constructor)
      .CreateConstructor(&VMCoLearner::ConstructorFromId)
      .CreateSerializeDefaultConstructor([](VM *vm, TypeId type_id) -> Ptr<VMCoLearner> {
        return Ptr<VMCoLearner>{new VMCoLearner(vm, type_id)};
      })
      .CreateMemberFunction("setId", &VMCoLearner::SetId)
      .CreateMemberFunction("getId", &VMCoLearner::GetId)
      .CreateMemberFunction("getUpdateCount", &VMCoLearner::GetUpdateCount)
      .CreateMemberFunction("getUpdate", &VMCoLearner::GetUpdate)
      .CreateMemberFunction("pushUpdate", &VMCoLearner::PushUpdate)
      ;
}

VMCoLearner::CPPType& VMCoLearner::GetLearner()
{
  return *learner_;
}

}  // namespace dmlf
}  // namespace vm_modules
}  // namespace fetch


