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

struct IArray : public Object
{
  IArray() = delete;
  IArray(VM *vm, TypeId type_id)
    : Object(vm, type_id)
  {}
  virtual ~IArray() = default;
  static Ptr<IArray> Constructor(VM *vm, TypeId type_id, int32_t size);
};

template <typename T>
struct Array : public IArray
{
  Array() = delete;
  Array(VM *vm, TypeId type_id, int32_t size)
    : IArray(vm, type_id)
  {
    Init<T>(size_t(size));
  }
  virtual ~Array() = default;

  template <typename U, typename std::enable_if_t<IsPrimitive<U>::value> * = nullptr>
  void Init(size_t size)
  {
    elements = std::vector<T>(size, 0);
  }
  template <typename U, typename std::enable_if_t<IsPtr<U>::value> * = nullptr>
  void Init(size_t size)
  {
    elements = std::vector<T>(size);
  }
  T *Find()
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
    T *ptr = Find();
    if (ptr)
    {
      Variant &top = Push();
      top.Construct(*ptr, element_type_id);
    }
  }

  virtual void PopToElement() override
  {
    T *ptr = Find();
    if (ptr)
    {
      Variant &top = Pop();
      *ptr         = top.Move<T>();
    }
  }

  std::vector<T> elements;
};

inline Ptr<IArray> IArray::Constructor(VM *vm, TypeId type_id, int32_t size)
{
  TypeInfo const &type_info       = vm->GetTypeInfo(type_id);
  TypeId const    element_type_id = type_info.parameter_type_ids[0];
  if (size < 0)
  {
    vm->RuntimeError("negative size");
    return nullptr;
  }
  switch (element_type_id)
  {
  case TypeIds::Bool:
  {
    return Ptr<IArray>(new Array<uint8_t>(vm, type_id, size));
  }
  case TypeIds::Int8:
  {
    return Ptr<IArray>(new Array<int8_t>(vm, type_id, size));
  }
  case TypeIds::Byte:
  {
    return Ptr<IArray>(new Array<uint8_t>(vm, type_id, size));
  }
  case TypeIds::Int16:
  {
    return Ptr<IArray>(new Array<int16_t>(vm, type_id, size));
  }
  case TypeIds::UInt16:
  {
    return Ptr<IArray>(new Array<uint16_t>(vm, type_id, size));
  }
  case TypeIds::Int32:
  {
    return Ptr<IArray>(new Array<int32_t>(vm, type_id, size));
  }
  case TypeIds::UInt32:
  {
    return Ptr<IArray>(new Array<uint32_t>(vm, type_id, size));
  }
  case TypeIds::Int64:
  {
    return Ptr<IArray>(new Array<int64_t>(vm, type_id, size));
  }
  case TypeIds::UInt64:
  {
    return Ptr<IArray>(new Array<uint64_t>(vm, type_id, size));
  }
  case TypeIds::Float32:
  {
    return Ptr<IArray>(new Array<float>(vm, type_id, size));
  }
  case TypeIds::Float64:
  {
    return Ptr<IArray>(new Array<double>(vm, type_id, size));
  }
  default:
  {
    return Ptr<IArray>(new Array<Ptr<Object>>(vm, type_id, size));
  }
  }  // switch
}

}  // namespace vm
}  // namespace fetch
