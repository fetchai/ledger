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

#include "meta/type_util.hpp"

#include <type_traits>

namespace fetch {
namespace vm {

template <typename T>
using IsResult = std::conditional_t<std::is_same<T, void>::value, std::integral_constant<int, 0>,
                                    std::integral_constant<int, 1>>;

template <typename T>
struct StackGetter
{
  static T Get(VM *vm, int sp_offset)
  {
    Variant &v = vm->stack_[vm->sp_ - sp_offset];
    return v.Move<T>();
  }
};

template <typename T>
struct StackSetter
{
  static void Set(VM *vm, int sp_offset, T &&result, TypeId type_id)
  {
    Variant &v = vm->stack_[vm->sp_ - sp_offset];
    v.Assign(std::forward<T>(result), type_id);
  }
};

template <typename T, typename = void>
struct TypeGetter;
template <typename T>
struct TypeGetter<T, typename std::enable_if_t<IsPrimitive<T>::value || IsVariant<T>::value>>
{
  static TypeIndex GetTypeIndex()
  {
    return TypeIndex(typeid(T));
  }
};
template <typename T>
struct TypeGetter<T, typename std::enable_if_t<IsPtr<T>::value>>
{
  static TypeIndex GetTypeIndex()
  {
    using ManagedType = typename GetManagedType<T>::type;
    return TypeIndex(typeid(ManagedType));
  }
};

template <typename T>
struct TypeGetter<T, typename std::enable_if_t<IsAddress<T>::value>>
{
  static TypeIndex GetTypeIndex()
  {
    using ManagedType = typename GetManagedType<Ptr<T>>::type;
    return TypeIndex(typeid(ManagedType));
  }
};

template <typename T, typename = void>
struct ParameterTypeGetter;
template <typename T>
struct ParameterTypeGetter<
    T, typename std::enable_if_t<IsPrimitiveParameter<T>::value || IsVariantParameter<T>::value>>
{
  static TypeIndex GetTypeIndex()
  {
    return TypeIndex(typeid(std::decay_t<T>));
  }
};
template <typename T>
struct ParameterTypeGetter<T, typename std::enable_if_t<IsPtrParameter<T>::value>>
{
  static TypeIndex GetTypeIndex()
  {
    using ManagedType = typename GetManagedType<std::decay_t<T>>::type;
    return TypeIndex(typeid(ManagedType));
  }
};

namespace detail_ {

template <template <typename...> class Getter, typename... Ts>
struct UnrollTypesWith
{
  static void Unroll(TypeIndexArray &array)
  {
    static const TypeIndex type_indices[] = {Getter<Ts>::GetTypeIndex()...};
    array.insert(array.end(), type_indices,
                 type_indices + sizeof(type_indices) / sizeof(type_indices[0]));
  }
};

template <template <typename...> class Getter>
struct UnrollTypesWith<Getter>
{
  static void Unroll(TypeIndexArray &)
  {}
};

}  // namespace detail_

template <typename... Ts>
using UnrollTypes = detail_::UnrollTypesWith<TypeGetter, Ts...>;

template <typename Instantiation>
struct UnrollTemplateParameters;
template <template <typename...> class Template, typename... Ts>
struct UnrollTemplateParameters<Template<Ts...>> : UnrollTypes<Ts...>
{
};

template <typename... Ts>
using UnrollParameterTypes = detail_::UnrollTypesWith<ParameterTypeGetter, Ts...>;

template <typename T>
using MakeParameterType =
    type_util::Switch<IsPrimitive<T>, T, IsVariant<T>, T const &, IsPtr<T>, T const &>;

template <typename Type, typename OutputType, typename... InputTypes>
struct IndexedValueGetter;
template <typename Type, typename OutputType, typename... InputTypes>
struct IndexedValueGetter<Type, std::tuple<InputTypes...>, OutputType>
{
  using type = OutputType (Type::*)(typename fetch::vm::MakeParameterType<InputTypes>::type...);
};

template <typename Type, typename OutputType, typename... InputTypes>
struct IndexedValueSetter;
template <typename Type, typename OutputType, typename... InputTypes>
struct IndexedValueSetter<Type, std::tuple<InputTypes...>, OutputType>
{
  using type = void (Type::*)(typename fetch::vm::MakeParameterType<InputTypes>::type...,
                              typename fetch::vm::MakeParameterType<OutputType>::type);
};

template <typename F>
struct FunctorTraits;
template <typename F>
struct FunctorTraits : FunctorTraits<decltype(&std::remove_reference_t<F>::operator())>
{
};
template <typename ReturnType, typename Class_, typename... Args>
struct FunctorTraits<ReturnType (Class_::*)(Args...) const>
{
  using class_type      = Class_;
  using return_type     = ReturnType;
  using args_tuple_type = std::tuple<Args...>;
};
// Support mutable lambdas
template <typename ReturnType, typename Class_, typename... Args>
struct FunctorTraits<ReturnType (Class_::*)(Args...)>
{
  using class_type      = Class_;
  using return_type     = ReturnType;
  using args_tuple_type = std::tuple<Args...>;
};

}  // namespace vm
}  // namespace fetch
