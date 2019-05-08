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
  virtual TemplateParameter GetIndexedValue(AnyInteger const &index) = 0;
  virtual void SetIndexedValue(AnyInteger const &index, TemplateParameter const &value) = 0;

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

  Array(VM *vm, TypeId type_id, TypeId element_type_id, int32_t size)
    : IArray(vm, type_id)
  {
    element_type_id_ = element_type_id;
    elements_ = std::vector<ElementType>(size_t(size), 0);
  }

  virtual int32_t Count() const override
  {
    return int32_t(elements_.size());
  }

  virtual TemplateParameter GetIndexedValue(AnyInteger const &index) override
  {
    ElementType *ptr = Find(index);
    if (ptr)
    {
      return TemplateParameter(*ptr, element_type_id_);
    }
    // Not found
    return TemplateParameter();
  }

  virtual void SetIndexedValue(AnyInteger const &index, TemplateParameter const &value) override
  {
    ElementType *ptr = Find(index);
    if (ptr)
    {
      *ptr         = value.Get<ElementType>();
    }
  }

  ElementType *Find(Variant const &index)
  {
    size_t   i;
    if (GetNonNegativeInteger(index, i) == false)
    {
      RuntimeError("negative index");
      return nullptr;
    }
    if (i >= elements_.size())
    {
      RuntimeError("index out of bounds");
      return nullptr;
    }
    ElementType &element = elements_[i];
    return &element;
  }

  TypeId element_type_id_;
  std::vector<ElementType> elements_;
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
    return new Array<uint8_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int8:
  {
    return new Array<int8_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Byte:
  {
    return new Array<uint8_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int16:
  {
    return new Array<int16_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::UInt16:
  {
    return new Array<uint16_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int32:
  {
    return new Array<int32_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::UInt32:
  {
    return new Array<uint32_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int64:
  {
    return new Array<int64_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::UInt64:
  {
    return new Array<uint64_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Float32:
  {
    return new Array<float>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Float64:
  {
    return new Array<double>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  default:
  {
    return new Array<Ptr<Object>>(vm, type_id, element_type_id, std::forward<Args>(args)...);
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
