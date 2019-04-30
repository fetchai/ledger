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

public:
  template <typename... Args>
  static Ptr<IState> ConstructIntrinsic(VM *vm, TypeId type_id, TypeId value_type_id,
                                        Args &&... args);
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

inline IoObserverInterface::Status ReadHelper(std::string const &name, std::vector<uint8_t> &bytes,
                                              IoObserverInterface &io)
{
  auto const exists_status = io.Exists(name);
  if (exists_status != IoObserverInterface::Status::OK)
  {
    return exists_status;
  }

  uint64_t buffer_size = bytes.size();
  auto     result      = io.Read(name, bytes.data(), buffer_size);

  if (IoObserverInterface::Status::OK == result)
  {
    // chop down the size of the buffer
    bytes.resize(buffer_size);
  }
  else if (IoObserverInterface::Status::BUFFER_TOO_SMALL == result)
  {
    // increase the buffer size
    bytes.resize(buffer_size);

    // make the second call to the io observer
    result = io.Read(name, bytes.data(), buffer_size);
  }

  return result;
}

inline IoObserverInterface::Status ReadHelper(std::string const &name, Ptr<Address> &val,
                                              IoObserverInterface &io)
{
  // create an initial buffer size
  std::vector<uint8_t> bytes(256, 0);
  auto                 result = ReadHelper(name, bytes, io);

  // get the type to convert itself
  val->FromBytes(std::move(bytes));

  return result;
}

inline IoObserverInterface::Status ReadHelper(std::string const &name, Ptr<String> &val,
                                              IoObserverInterface &io)
{
  // create an initial buffer size
  std::vector<uint8_t> bytes(256, 0);
  auto                 result = ReadHelper(name, bytes, io);

  // get the type to convert itself
  val->str.assign(reinterpret_cast<char const *>(bytes.data()), bytes.size());

  return result;
}

inline IoObserverInterface::Status WriteHelper(std::string const &name, Ptr<Address> const &val,
                                               IoObserverInterface &io)
{
  // convert the object to bytes
  auto bytes = val->ToBytes();

  return io.Write(name, bytes.data(), bytes.size());
}

inline IoObserverInterface::Status WriteHelper(std::string const &name, Ptr<String> const &val,
                                               IoObserverInterface &io)
{
  auto const &str = val->str;
  // convert the object to bytes
  return io.Write(name, str.c_str(), str.size());
}

template <typename T>
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
    }
  }

  ~State() override = default;

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
      WriteHelper(name_->str, value_, vm_->GetIOObserver());
    }
  }

  Ptr<String> name_;
  Value       value_;
  TypeId      value_type_id_;
  bool        existed_{false};
};

template <typename... Args>
inline Ptr<IState> IState::ConstructIntrinsic(VM *vm, TypeId type_id, TypeId value_type_id,
                                              Args &&... args)
{
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
  case TypeIds::String:
  {
    return new State<String>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Address:
  {
    return new State<Address>(vm, type_id, value_type_id, std::forward<Args>(args)...);
  }
  }  // switch

  vm->RuntimeError("Unsupported type passed as template parameter to State<T>");
  return nullptr;
}

template <typename... Args>
inline Ptr<IState> IState::Construct(VM *vm, TypeId type_id, Args &&... args)
{
  TypeInfo const &type_info     = vm->GetTypeInfo(type_id);
  TypeId const    value_type_id = type_info.parameter_type_ids[0];
  return ConstructIntrinsic(vm, type_id, value_type_id, std::forward<Args>(args)...);
}

inline Ptr<IState> IState::Constructor(VM *vm, TypeId type_id, Ptr<String> const &name,
                                       TemplateParameter const &value)
{
  return Construct(vm, type_id, name, value);
}

inline Ptr<IState> IState::Constructor(VM *vm, TypeId type_id, Ptr<Address> const &address,
                                       TemplateParameter const &value)
{
  return Construct(vm, type_id, address->AsBase64String(), value);
}

}  // namespace vm
}  // namespace fetch
