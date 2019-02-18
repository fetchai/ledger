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

class IArray : public Object
{
public:
  IArray()          = delete;
  virtual ~IArray() = default;
  static Ptr<IArray> Constructor(VM *vm, TypeId type_id, int32_t size);
  virtual int32_t    Count() const = 0;

protected:
  IArray(VM *vm, TypeId type_id)
    : Object(vm, type_id)
  {}
  template <typename... Args>
  static Ptr<IArray> Construct(VM *vm, TypeId type_id, Args &&... args);
};

template <typename T>
struct Array : public IArray
{
  using ElementType = typename GetStorageType<T>::type;

  Array()          = delete;
  virtual ~Array() = default;

  Array(VM *vm, TypeId type_id, int32_t size)
    : IArray(vm, type_id)
  {
    elements = std::vector<ElementType>(size_t(size), 0);
  }

  virtual int32_t Count() const override
  {
    return int32_t(elements.size());
  }

  ElementType *Find()
  {
    Variant &positionv = Pop();
    size_t   position;
    if (GetInteger(positionv, position) == false)
    {
      RuntimeError("negative index");
      return nullptr;
    }
    positionv.Reset();
    if (position >= elements.size())
    {
      RuntimeError("index out of bounds");
      return nullptr;
    }
    return &elements[position];
  }

  virtual void *FindElement() override
  {
    return Find();
  }

  virtual void PushElement(TypeId element_type_id) override
  {
    ElementType *ptr = Find();
    if (ptr)
    {
      Variant &top = Push();
      top.Construct(*ptr, element_type_id);
    }
  }

  virtual void PopToElement() override
  {
    ElementType *ptr = Find();
    if (ptr)
    {
      Variant &top = Pop();
      *ptr         = top.Move<ElementType>();
    }
  }

  std::vector<ElementType> elements;
};

template <typename... Args>
inline Ptr<IArray> IArray::Construct(VM *vm, TypeId type_id, Args &&... args)
{
  TypeInfo const &type_info       = vm->GetTypeInfo(type_id);
  TypeId const    element_type_id = type_info.parameter_type_ids[0];
  switch (element_type_id)
  {
  case TypeIds::Bool:
  {
    return new Array<uint8_t>(vm, type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int8:
  {
    return new Array<int8_t>(vm, type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Byte:
  {
    return new Array<uint8_t>(vm, type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int16:
  {
    return new Array<int16_t>(vm, type_id, std::forward<Args>(args)...);
  }
  case TypeIds::UInt16:
  {
    return new Array<uint16_t>(vm, type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int32:
  {
    return new Array<int32_t>(vm, type_id, std::forward<Args>(args)...);
  }
  case TypeIds::UInt32:
  {
    return new Array<uint32_t>(vm, type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int64:
  {
    return new Array<int64_t>(vm, type_id, std::forward<Args>(args)...);
  }
  case TypeIds::UInt64:
  {
    return new Array<uint64_t>(vm, type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Float32:
  {
    return new Array<float>(vm, type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Float64:
  {
    return new Array<double>(vm, type_id, std::forward<Args>(args)...);
  }
  default:
  {
    return new Array<Ptr<Object>>(vm, type_id, std::forward<Args>(args)...);
  }
  }  // switch
}

inline Ptr<IArray> IArray::Constructor(VM *vm, TypeId type_id, int32_t size)
{
  if (size < 0)
  {
    vm->RuntimeError("negative size");
    return Ptr<IArray>();
  }
  return Construct(vm, type_id, size);
}

}  // namespace vm
}  // namespace fetch
