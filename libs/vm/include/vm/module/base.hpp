#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include <type_traits>
#include <typeinfo>
#include <utility>

namespace fetch {
namespace vm {

template <typename T>
struct StackGetter
{
  using DecayedT = std::decay_t<T>;

  static DecayedT Get(VM *vm, int sp_offset)
  {
    Variant &v = vm->stack_[vm->sp_ - sp_offset];
    return v.Move<DecayedT>();
  }
};

template <typename T>
struct StackSetter
{
  static void Set(VM *vm, int result_sp, T &&result, TypeId type_id)
  {
    Variant &v = vm->stack_[result_sp];
    v.Assign(std::forward<T>(result), type_id);
  }
};

template <typename T, typename = void>
struct TypeGetter;
template <typename T>
struct TypeGetter<T, std::enable_if_t<IsPrimitive<T> || IsVariant<T>>>
{
  static TypeIndex GetTypeIndex()
  {
    return TypeIndex(typeid(T));
  }
};
template <typename T>
struct TypeGetter<T, std::enable_if_t<IsPtr<T>>>
{
  static TypeIndex GetTypeIndex()
  {
    using ManagedType = GetManagedType<T>;
    return TypeIndex(typeid(ManagedType));
  }
};

template <typename T>
struct TypeGetter<T, std::enable_if_t<IsAddress<T>>>
{
  static TypeIndex GetTypeIndex()
  {
    using ManagedType = GetManagedType<Ptr<T>>;
    return TypeIndex(typeid(ManagedType));
  }
};

template <typename T, typename = void>
struct ParameterTypeGetter;
template <typename T>
struct ParameterTypeGetter<
    T, std::enable_if_t<IsPrimitiveParameter<T>::value || IsVariantParameter<T>::value>>
{
  static TypeIndex GetTypeIndex()
  {
    return TypeIndex(typeid(std::decay_t<T>));
  }
};
template <typename T>
struct ParameterTypeGetter<T, std::enable_if_t<IsPtrParameter<T>::value>>
{
  static TypeIndex GetTypeIndex()
  {
    using ManagedType = GetManagedType<std::decay_t<T>>;
    return TypeIndex(typeid(ManagedType));
  }
};

template <typename... Ts>
struct UnrollTypes;
template <typename T, typename... Ts>
struct UnrollTypes<T, Ts...>
{
  // Invoked on non-final type
  static void Unroll(TypeIndexArray &array)
  {
    array.emplace_back(TypeGetter<T>::GetTypeIndex());
    UnrollTypes<Ts...>::Unroll(array);
  }
};
template <>
struct UnrollTypes<>
{
  // Invoked on zero types
  static void Unroll(TypeIndexArray & /* array */)
  {}
};

template <typename... Ts>
struct UnrollTemplateParameters;
template <template <typename, typename...> class Template, typename... Ts>
struct UnrollTemplateParameters<Template<Ts...>>
{
  static void Unroll(TypeIndexArray &array)
  {
    UnrollTypes<Ts...>::Unroll(array);
  }
};

template <typename... Ts>
struct UnrollParameterTypes;
template <typename T, typename... Ts>
struct UnrollParameterTypes<T, Ts...>
{
  // Invoked on non-final type
  static void Unroll(TypeIndexArray &array)
  {
    array.emplace_back(ParameterTypeGetter<T>::GetTypeIndex());
    UnrollParameterTypes<Ts...>::Unroll(array);
  }
};
template <>
struct UnrollParameterTypes<>
{
  // Invoked on zero types
  static void Unroll(TypeIndexArray & /* array */)
  {}
};

template <typename Tuple>
using UnrollTupleParameterTypes = meta::UnpackTuple<Tuple, UnrollParameterTypes>;

template <typename T, typename = void>
struct MakeParameterType;
template <typename T>
struct MakeParameterType<T, std::enable_if_t<fetch::vm::IsPrimitive<T>>>
{
  using type = T;
};
template <typename T>
struct MakeParameterType<T, std::enable_if_t<fetch::vm::IsVariant<T>>>
{
  using type = T const &;
};
template <typename T>
struct MakeParameterType<T, std::enable_if_t<fetch::vm::IsPtr<T>>>
{
  using type = T const &;
};

}  // namespace vm
}  // namespace fetch
