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

#include "core/serializers/byte_array_buffer.hpp"

#include "vm/address.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm {

class IState : public Object
{
public:
  IState()          = delete;
  virtual ~IState() = default;

  static Ptr<IState> Constructor(VM *vm, TypeId type_id, Ptr<String> const &name,
                                 TemplateParameter const &value);

  static Ptr<IState> Constructor(VM *vm, TypeId type_id, Ptr<Address> const &address,
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

template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
inline IoObserverInterface::Status ReadHelper(std::string const &name, T &val,
                                              IoObserverInterface &io)
{
  uint64_t buffer_size = sizeof(T);
  return io.Read(name, &val, buffer_size);
}

template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
inline IoObserverInterface::Status WriteHelper(std::string const &name, T const &val,
                                               IoObserverInterface &io)
{
  return io.Write(name, &val, sizeof(T));
}

inline IoObserverInterface::Status ReadHelper(std::string const &name, Ptr<Object> &val,
                                              IoObserverInterface &io)
{
  using fetch::byte_array::ByteArray;
  using fetch::serializers::ByteArrayBuffer;

  auto const exists_status = io.Exists(name);
  if (exists_status != IoObserverInterface::Status::OK)
  {
    return exists_status;
  }

  // create an initial buffer size
  ByteArray buffer;
  buffer.Resize(256);

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

  // if we successfully extracted the data
  if (IoObserverInterface::Status::OK == result)
  {
    // cretae the byte array buffer
    ByteArrayBuffer byte_buffer{buffer};

    // attempt to deserialize the value from the stream
    if (!val->DeserializeFrom(byte_buffer))
    {
      result = IoObserverInterface::Status::ERROR;
    }
  }

  return result;
}

inline IoObserverInterface::Status WriteHelper(std::string const &name, Ptr<Object> const &val,
                                               IoObserverInterface &io)
{
  using fetch::serializers::ByteArrayBuffer;

  // convert the type into a byte stream
  ByteArrayBuffer buffer;
  if (!val || !val->SerializeTo(buffer))
  {
    return IoObserverInterface::Status::ERROR;
  }

  return io.Write(name, buffer.data().pointer(), buffer.data().size());
}

template <typename T, typename = void>
class State : public IState
{
public:
  // Construct state object, default argument = get from state DB, initializing to value if not
  // found
  State(VM *vm, TypeId type_id, TypeId value_type_id, Ptr<String> name,
        TemplateParameter const &value)
    : IState(vm, type_id)
    , name_{std::move(name)}
    , value_{value.Get<Value>()}
    , value_type_id_{value_type_id}
  {
    // if we have a IO observer then
    if (vm_->HasIoObserver())
    {
      // attempt to read the value from the storage engine
      auto const status = ReadHelper(name_->str, value_, vm_->GetIOObserver());

      // mark the variable as existed if we get a positive result back
      existed_ = (Status::OK == status);
      if (!existed_)
      {
        Set(value);
      }
    }
  }

  ~State() override
  {
    try
    {
      FlushIO();
    }
    catch (...)
    {
      vm_->RuntimeError("An exception has been thrown from State<...>::FlushIO().");
    }
  }

  TemplateParameter Get() const override
  {
    return TemplateParameter(value_, value_type_id_);
  }

  void Set(TemplateParameter const &value) override
  {
    value_ = GetValue<>(value);
  }

  bool Existed() const override
  {
    return existed_;
  }

private:
  using Value  = typename GetStorageType<T>::type;
  using Status = IoObserverInterface::Status;

  template <typename Y = T>
  meta::EnableIf<IsPrimitive<Y>::value, Y> GetValue(TemplateParameter const &value)
  {
    return value.Get<Value>();
  }

  template <typename Y = T>
  meta::EnableIf<IsPtr<Y>::value, Y> GetValue(TemplateParameter const &value)
  {
    auto v{value.Get<Value>()};
    if (!v)
    {
      vm_->RuntimeError("Input value is null reference.");
    }
    return v;
  }

  void FlushIO()
  {
    // if we have an IO observer then inform it of the changes
    if (!vm_->HasError() && vm_->HasIoObserver())
    {
      WriteHelper(name_->str, value_, vm_->GetIOObserver());
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
    return new State<Ptr<Object>>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  }  // switch
}

inline Ptr<IState> IState::Constructor(VM *vm, TypeId type_id, Ptr<String> const &name,
                                       TemplateParameter const &value)
{
  if (name)
  {
    return Construct(vm, type_id, name, value);
  }

  vm->RuntimeError("Failed to construct State: name is null");
  return nullptr;
}

inline Ptr<IState> IState::Constructor(VM *vm, TypeId type_id, Ptr<Address> const &address,
                                       TemplateParameter const &value)
{
  if (address)
  {
    return Construct(vm, type_id, address->AsString(), value);
  }

  vm->RuntimeError("Failed to construct State: address is null");
  return nullptr;
}

}  // namespace vm
}  // namespace fetch
