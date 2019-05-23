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

namespace fetch {
namespace vm {

template <typename T>
struct IsResult
{
  static const int value = 1;
};
template <>
struct IsResult<void>
{
  static const int value = 0;
};

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

template <typename... Ts>
struct UnrollTypes;
template <typename T, typename... Ts>
struct UnrollTypes<T, Ts...>
{
  // Invoked on non-final type
  static void Unroll(TypeIndexArray &array)
  {
    TypeIndex const type_index = TypeGetter<T>::GetTypeIndex();
    array.push_back(type_index);
    UnrollTypes<Ts...>::Unroll(array);
  }
};
template <typename T>
struct UnrollTypes<T>
{
  // Invoked on final type
  static void Unroll(TypeIndexArray &array)
  {
    TypeIndex const type_index = TypeGetter<T>::GetTypeIndex();
    array.push_back(type_index);
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
    TypeIndex const type_index = ParameterTypeGetter<T>::GetTypeIndex();
    array.push_back(type_index);
    UnrollParameterTypes<Ts...>::Unroll(array);
  }
};
template <typename T>
struct UnrollParameterTypes<T>
{
  // Invoked on final type
  static void Unroll(TypeIndexArray &array)
  {
    TypeIndex const type_index = ParameterTypeGetter<T>::GetTypeIndex();
    array.push_back(type_index);
  }
};
template <>
struct UnrollParameterTypes<>
{
  // Invoked on zero types
  static void Unroll(TypeIndexArray & /* array */)
  {}
};

template <typename... Ts>
struct UnrollTupleParameterTypes;
template <typename... Ts>
struct UnrollTupleParameterTypes<std::tuple<Ts...>>
{
  static void Unroll(TypeIndexArray &array)
  {
    UnrollParameterTypes<Ts...>::Unroll(array);
  }
};

template <typename T, typename = void>
struct MakeParameterType;
template <typename T>
struct MakeParameterType<T, typename std::enable_if_t<fetch::vm::IsPrimitive<T>::value>>
{
  using type = T;
};
template <typename T>
struct MakeParameterType<T, typename std::enable_if_t<fetch::vm::IsVariant<T>::value>>
{
  using type = T const &;
};
template <typename T>
struct MakeParameterType<T, typename std::enable_if_t<fetch::vm::IsPtr<T>::value>>
{
  using type = T const &;
};

template <typename T, typename... Ts>
struct RemoveFirstType;
template <typename T, typename... Ts>
struct RemoveFirstType<std::tuple<T, Ts...>>
{
  using type = typename std::tuple<Ts...>;
};

template <typename... Ts>
struct RemoveLastType;
template <typename... Ts>
struct RemoveLastType<std::tuple<Ts...>>
{
  template <size_t... Is>
  static std::tuple<std::tuple_element_t<Is, std::tuple<Ts...>>...> f(std::index_sequence<Is...>);
  using type = decltype(f(std::make_index_sequence<sizeof...(Ts) - 1>()));
};

template <typename... Ts>
struct GetLastType;
template <typename... Ts>
struct GetLastType<std::tuple<Ts...>>
{
  using type = typename std::tuple_element_t<sizeof...(Ts) - 1, std::tuple<Ts...>>;
};

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

template <typename T>
struct FunctorReturnTypeExtractor : public FunctorReturnTypeExtractor<decltype(&T::operator())>
{
};
template <typename Functor, typename ReturnType, typename... Ts>
struct FunctorReturnTypeExtractor<ReturnType (Functor::*)(Ts...) const>
{
  using type = ReturnType;
};
template <typename T>
struct FunctorSignatureExtractor : public FunctorSignatureExtractor<decltype(&T::operator())>
{
};
template <typename Functor, typename ReturnType, typename... Ts>
struct FunctorSignatureExtractor<ReturnType (Functor::*)(Ts...) const>
{
  using type = std::tuple<Ts...>;
};

}  // namespace vm
}  // namespace fetch
