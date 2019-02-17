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
  IState() = delete;
  virtual ~IState() = default;
  static Ptr<IState> Constructor(VM *vm, TypeId type_id, Ptr<String> const &name);
  static Ptr<IState> Constructor(VM *vm, TypeId type_id, Ptr<String> const &name,
    TemplateParameter const &value);
  virtual TemplateParameter Get() const = 0;
  virtual void Set(TemplateParameter const &value) = 0;

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
  State() = delete;
  virtual ~State() = default;

  State(VM *vm, TypeId type_id, TypeId value_type_id, Ptr<String> const &name)
    : IState(vm, type_id)
  {
    value_ = Value(0);
    value_type_id_ = value_type_id;
    name_ = name->str;
  }

  State(VM *vm, TypeId type_id, TypeId value_type_id, Ptr<String> const &name,
      TemplateParameter const &value)
    : IState(vm, type_id)
  {
    value_ = value.Get<Value>();
    value_type_id_ = value_type_id;
    name_ = name->str;
  }

  virtual TemplateParameter Get() const override
  {
    return TemplateParameter(value_, value_type_id_);
  }

  virtual void Set(TemplateParameter const &value) override
  {
    value_ = value.Get<Value>();
  }

private:
  using Value = typename GetStorageType<T>::type;
  std::string name_;
  Value   value_;
  TypeId value_type_id_;
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
    return new State<Ptr<Object>>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  } // switch
}

inline Ptr<IState> IState::Constructor(VM *vm, TypeId type_id, Ptr<String> const &name)
{
  return Construct(vm, type_id, name);
}

inline Ptr<IState> IState::Constructor(VM *vm, TypeId type_id, Ptr<String> const &name,
   TemplateParameter const &value)
{
  return Construct(vm, type_id, name, value);
}

}  // namespace vm
}  // namespace fetch
