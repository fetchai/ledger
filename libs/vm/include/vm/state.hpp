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

#include "vm/vm.hpp"

namespace fetch {
namespace vm {

class IState : public Object
{
public:
  IState()          = delete;
  virtual ~IState() = default;
  static Ptr<IState>        Constructor(VM *vm, TypeId type_id, Ptr<String> const &name);
  static Ptr<IState>        Constructor(VM *vm, TypeId type_id, Ptr<String> const &name,
                                        TemplateParameter const &value);
  virtual TemplateParameter Get() const                         = 0;
  virtual void              Set(TemplateParameter const &value) = 0;
  virtual bool              Existed() const                     = 0;

protected:
  IState(VM *vm, TypeId type_id)
    : Object(vm, type_id)
  {}
  template <typename... Args>
  static Ptr<IState> Construct(VM *vm, TypeId type_id, Args &&... args);
};

template <typename T>
class State : public IState
{
public:
  // Important restriction for the moment is that the state value must be primitive
  static_assert(IsPrimitive<T>::value, "State value must be a primitive");

  // Construct state object, default argument = get from state DB, initializing to value if not
  // found
  State(VM *vm, TypeId type_id, TypeId value_type_id, Ptr<String> const &name,
        TemplateParameter const &value)
    : IState(vm, type_id)
    , name_{name}
    , value_{value.Get<Value>()}
    , value_type_id_{value_type_id}
  {
    // if we have a IO observer then
    if (vm_->HasIoObserver())
    {
      // attempt to read the value from the storage engine
      uint64_t   buffer_size = sizeof(value_);  // less important now with primitive types
      auto const status      = vm_->GetIOObserver().Read(name_->str, &value_, buffer_size);

      // mark the variable as existed if we get a positive result back
      existed_ = (Status::OK == status);
    }
  }

  ~State() override
  {}

  TemplateParameter Get() const override
  {
    return TemplateParameter(value_, value_type_id_);
  }

  void Set(TemplateParameter const &value) override
  {
    value_ = value.Get<Value>();

    // flush the value if it is being observed
    FlushIO();
  }

  bool Existed() const override
  {
    return existed_;
  }

private:
  using Value  = typename GetStorageType<T>::type;
  using Status = IoObserverInterface::Status;

  void FlushIO()
  {
    // if we have an IO observer then inform it of the changes
    if (vm_->HasIoObserver())
    {
      vm_->GetIOObserver().Write(name_->str, &value_, sizeof(value_));
    }
  }

  Ptr<String> name_;
  Value       value_;
  TypeId      value_type_id_;
  bool        existed_{false};
};

template <typename... Args>
inline Ptr<IState> IState::Construct(VM *vm, TypeId type_id, Args &&... args)
{
  TypeInfo const &type_info     = vm->GetTypeInfo(type_id);
  TypeId const    value_type_id = type_info.parameter_type_ids[0];

  switch (value_type_id)
  {
  case TypeIds::Bool:
  {
    return new State<uint8_t>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int8:
  {
    return new State<int8_t>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Byte:
  {
    return new State<uint8_t>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int16:
  {
    return new State<int16_t>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::UInt16:
  {
    return new State<uint16_t>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int32:
  {
    return new State<int32_t>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::UInt32:
  {
    return new State<uint32_t>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int64:
  {
    return new State<int64_t>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::UInt64:
  {
    return new State<uint64_t>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Float32:
  {
    return new State<float>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Float64:
  {
    return new State<double>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  default:
  {
    throw std::runtime_error("Unsupported State type");
  }
  }  // switch
}

inline Ptr<IState> IState::Constructor(VM *vm, TypeId type_id, Ptr<String> const &name,
                                       TemplateParameter const &value)
{
  return Construct(vm, type_id, name, value);
}

}  // namespace vm
}  // namespace fetch
