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

#include "vm/state.hpp"

namespace fetch {
namespace vm {

namespace {
template <typename T, typename = std::enable_if_t<IsPrimitive<T>::value>>
inline bool ReadHelper(TypeId /*type*/, std::string const &name, T &val, VM *vm)
{
  if (!vm->HasIoObserver())
  {
    return true;
  }

  uint64_t   buffer_size = sizeof(T);
  auto const result      = vm->GetIOObserver().Read(name, &val, buffer_size);
  return result == IoObserverInterface::Status::OK;
}

template <typename T, typename = std::enable_if_t<IsPrimitive<T>::value>>
inline bool WriteHelper(std::string const &name, T const &val, VM *vm)
{
  if (!vm->HasIoObserver())
  {
    return true;
  }

  auto const result = vm->GetIOObserver().Write(name, &val, sizeof(T));
  return result == IoObserverInterface::Status::OK;
}

inline bool ReadHelper(TypeId type, std::string const &name, Ptr<Object> &val, VM *vm)
{
  using fetch::byte_array::ByteArray;
  using fetch::serializers::ByteArrayBuffer;

  if (!vm->HasIoObserver())
  {
    return false;
  }

  if (!vm->IsDefaultSerializeConstructable(type))
  {
    vm->RuntimeError("Cannot deserialise object of type " + vm->GetUniqueId(type) +
                     " for which no serialisation constructor exists.");
  }

  val = vm->DefaultSerializeConstruct(type);

  // create an initial buffer size
  ByteArray buffer;
  buffer.Resize(256);

  IoObserverInterface &io{vm->GetIOObserver()};

  uint64_t buffer_size = buffer.size();
  auto     result      = io.Read(name, buffer.pointer(), buffer_size);

  if (IoObserverInterface::Status::OK == result)
  {
    // chop down the size of the buffer
    buffer.Resize(buffer_size);
  }
  else if (IoObserverInterface::Status::BUFFER_TOO_SMALL == result)
  {
    // increase the buffer size
    buffer.Resize(buffer_size);

    // make the second call to the io observer
    result = io.Read(name, buffer.pointer(), buffer_size);
  }

  bool retval = false;

  // if we successfully extracted the data
  if (IoObserverInterface::Status::OK == result)
  {
    ByteArrayBuffer byte_buffer{buffer};

    retval = val->DeserializeFrom(byte_buffer);
    if (!retval)
    {
      if (!vm->HasError())
      {
        vm->RuntimeError("Object deserialisation failed");
      }
    }
  }

  return retval;
}

inline bool WriteHelper(std::string const &name, Ptr<Object> const &val, VM *vm)
{
  using fetch::serializers::ByteArrayBuffer;

  if (!vm->HasIoObserver())
  {
    return true;
  }

  // convert the type into a byte stream
  ByteArrayBuffer buffer;
  if (val == nullptr)
  {
    vm->RuntimeError("Cannot serialise null reference");
  }

  if (!val->SerializeTo(buffer))
  {
    if (!vm->HasError())
    {
      vm->RuntimeError("Serialisation of object failed.");
    }
    return false;
  }

  auto const result =
      vm->GetIOObserver().Write(name, buffer.data().pointer(), buffer.data().size());
  return result == IoObserverInterface::Status::OK;
}

enum class eModifStatus : uint8_t
{
  deserialised,
  modified,
  undefined,
};

enum class eExisted : uint8_t
{
  yes,
  no,
  undefined,
};

template <typename T>
class State : public IState
{
public:
  // Construct state object, default argument = get from state DB, initializing to value if not
  // found
  State(VM *vm, TypeId type_id, TypeId template_param_type_id, Ptr<String> const &name)
    : IState(vm, type_id)
    , name_{name->str}
    , template_param_type_id_{template_param_type_id}
  {}

  ~State() override
  {
    try
    {
      FlushIO();
    }
    catch (std::exception const &ex)
    {
      // TODO(issue 1094): Support for nested runtime error(s) and/or exception(s)
      vm_->RuntimeError("An exception has been thrown from State<...>::FlushIO(). Desc.: " +
                        std::string(ex.what()));
    }
    catch (...)
    {
      // TODO(issue 1094): Support for nested runtime error(s) and/or exception(s)
      vm_->RuntimeError("An exception has been thrown from State<...>::FlushIO().");
    }
  }

  TemplateParameter1 Get() override
  {
    return GetInternal();
  }

  TemplateParameter1 Get(TemplateParameter1 const &default_value) override
  {
    return GetInternal(&default_value);
  }

  void Set(TemplateParameter1 const &value) override
  {
    SetValue<>(value);
  }

  bool Existed() override
  {
    if (eExisted::undefined == existed_ && vm_->HasIoObserver())
    {
      // mark the variable as existed if we get a positive result back
      existed_ = eExisted::no;
      if (Status::OK == vm_->GetIOObserver().Exists(name_))
      {
        existed_ = eExisted::yes;
      }
    }

    return existed_ == eExisted::yes;
  }

private:
  using Value  = typename GetStorageType<T>::type;
  using Status = IoObserverInterface::Status;

  TemplateParameter1 GetInternal(TemplateParameter1 const *default_value = nullptr)
  {
    if (mod_status_ != eModifStatus::undefined)
    {
      return {value_, template_param_type_id_};
    }
    else if (Existed())
    {
      if (ReadHelper(template_param_type_id_, name_, value_, vm_))
      {
        mod_status_ = eModifStatus::deserialised;
        return {value_, template_param_type_id_};
      }
    }
    else if (default_value)
    {
      return *default_value;
    }

    vm_->RuntimeError(
        "The state does not represent any value - value it has not been assigned and/or it does "
        "not exists in data storage.");

    return {};
  }

  template <typename Y = T>
  meta::EnableIf<IsPrimitive<Y>::value> SetValue(TemplateParameter1 const &value)
  {
    value_      = value.Get<Value>();
    mod_status_ = eModifStatus::modified;
  }

  template <typename Y = T>
  meta::EnableIf<IsPtr<Y>::value> SetValue(TemplateParameter1 const &value)
  {
    auto v{value.Get<Value>()};
    if (!v)
    {
      vm_->RuntimeError("Input value is null reference.");
    }
    value_      = std::move(v);
    mod_status_ = eModifStatus::modified;
  }

  void FlushIO()
  {
    // if we have an IO observer then inform it of the changes
    if (!vm_->HasError() && mod_status_ == eModifStatus::modified)
    {
      auto const result = WriteHelper(name_, value_, vm_);
      if (!result && !vm_->HasError())
      {
        vm_->RuntimeError("Failure of writing state to the storage.");
      }
    }
  }

  std::string  name_;
  TypeId       template_param_type_id_;
  Value        value_;
  eExisted     existed_{eExisted::undefined};
  eModifStatus mod_status_{eModifStatus::undefined};
};

template <typename... Args>
Ptr<IState> Construct(VM *vm, TypeId state_type_id, Args &&... args)
{
  TypeInfo const &state_type_info = vm->GetTypeInfo(state_type_id);
  TypeId const    value_type_id   = state_type_info.parameter_type_ids[0];
  return IState::ConstructIntrinsic(vm, state_type_id, value_type_id, std::forward<Args>(args)...);
}

template <typename T, typename R = void>
constexpr bool IsMetatype =
    !((!std::is_void<T>::value && IsPrimitive<T>::value) || IsPtr<T>::value);

template <typename T, typename = void>
struct StateFactory;

template <typename T>
struct StateFactory<T, std::enable_if_t<!IsMetatype<T>>>
{
  template <typename... Args>
  Ptr<IState> operator()(Args &&... args)
  {
    return new State<T>{std::forward<Args>(args)...};
  }
};

template <typename T>
struct StateFactory<T, std::enable_if_t<IsMetatype<T>>>
{
  template <typename... Args>
  Ptr<IState> operator()(Args &&...)
  {
    return {};
  }
};

}  // namespace

Ptr<IState> IState::Constructor(VM *vm, TypeId type_id, Ptr<String> const &name)
{
  if (name)
  {
    return Construct(vm, type_id, name);
  }

  vm->RuntimeError("Failed to construct State: the `name` is null reference");
  return nullptr;
}

Ptr<IState> IState::Constructor(VM *vm, TypeId type_id, Ptr<Address> const &name)
{
  if (name)
  {
    return Construct(vm, type_id, name->AsString());
  }

  vm->RuntimeError("Failed to construct State: the `name` is null reference");
  return nullptr;
}

Ptr<IState> IState::ConstructIntrinsic(VM *vm, TypeId type_id, TypeId template_param_type_id,
                                       Ptr<String> const &name)
{
  return TypeIdAsCanonicalType<StateFactory>(template_param_type_id, vm, type_id,
                                             template_param_type_id, name);
}

}  // namespace vm
}  // namespace fetch
