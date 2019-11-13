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

#include "vm/address.hpp"
#include "vm/module.hpp"

#include "vm_modules/dmlf/update.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace dmlf {

VMUpdate::VMUpdate(VM *vm, TypeId type_id)
  : Object(vm, type_id)
{
  update_ = std::make_unique<CPPType>();
}

VMUpdate::VMUpdate(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                   std::vector<CPPPayloadType> const gradients)
  : Object(vm, type_id)
{
  update_ = std::make_unique<CPPType>(std::move(gradients));
}

Ptr<VMUpdate> VMUpdate::Constructor(VM *vm, TypeId type_id)
{
  return Ptr<VMUpdate>{new VMUpdate(vm, type_id)};
}

Ptr<VMUpdate> VMUpdate::ConstructorFromVecPayload(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                  Ptr<Array<Ptr<VMPayloadType>>> const &payloads)
{
  std::size_t size = payloads->elements.size();

  std::vector<CPPPayloadType> gradients;
  gradients.resize(size);

  for (std::size_t i = 0; i < size; i++)
  {
    Ptr<VMPayloadType> tensor = payloads->elements.at(i);
    gradients.emplace_back(tensor->GetTensor());
  }

  return Ptr<VMUpdate>{new VMUpdate(vm, type_id, std::move(gradients))};
}

void VMUpdate::SetSource(Ptr<Address> const &addr)
{
  std::string pkey_b64{addr->address().display()};
  update_->SetSource(pkey_b64);
}

Ptr<Address> VMUpdate::GetSource()
{
  Ptr<Address> source =
      vm_->CreateNewObject<fetch::vm::Address>(Ptr<String>{new String{vm_, update_->GetSource()}});
  return source;
}

Ptr<Array<Ptr<VMUpdate::VMPayloadType>>> VMUpdate::GetGradients()
{
  std::vector<CPPPayloadType> const &gradients = update_->GetGradients();
  size_t                             size      = gradients.size();

  Ptr<Array<Ptr<VMPayloadType>>> payloads = vm_->CreateNewObject<Array<Ptr<VMPayloadType>>>(
      vm_->GetTypeId<VMPayloadType>(), static_cast<int32_t>(size));

  for (std::size_t i{0}; i < size; ++i)
  {
    Ptr<VMPayloadType> payload = vm_->CreateNewObject<VMPayloadType>(gradients[i]);
    payloads->elements[i]      = payload;
  }
  return payloads;
}

uint64_t VMUpdate::GetTimestamp()
{
  return update_->TimeStamp();
}

void VMUpdate::Bind(Module &module)
{
  module
      .CreateClassType<VMUpdate>("UpdatePacket")
      //.CreateConstructor(&VMUpdate::Constructor)
      .CreateConstructor(&VMUpdate::ConstructorFromVecPayload)
      .CreateSerializeDefaultConstructor([](VM *vm, TypeId type_id) -> Ptr<VMUpdate> {
        return Ptr<VMUpdate>{new VMUpdate(vm, type_id)};
      })
      .CreateMemberFunction("setSource", &VMUpdate::SetSource)
      .CreateMemberFunction("getSource", &VMUpdate::GetSource)
      .CreateMemberFunction("getGradients", &VMUpdate::GetGradients)
      .CreateMemberFunction("getTimestamp", &VMUpdate::GetTimestamp);
}

bool VMUpdate::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  buffer << *update_;
  return true;
}

bool VMUpdate::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  CPPTypePtr update = std::make_unique<CPPType>();
  buffer >> *update;

  this->update_ = std::move(update);

  return true;
}

VMUpdate::CPPType &VMUpdate::GetUpdate()
{
  return *update_;
}

void VMUpdate::SetUpdate(CPPType const &from)
{
  update_ = std::make_unique<CPPType>(from);
}

}  // namespace dmlf
}  // namespace vm_modules
}  // namespace fetch
