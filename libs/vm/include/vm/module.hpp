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

#include "meta/tuple.hpp"
#include "vm/compiler.hpp"
#include "vm/estimate_charge.hpp"
#include "vm/module/base.hpp"
#include "vm/module/constructor_invoke.hpp"
#include "vm/module/free_function_invoke.hpp"
#include "vm/module/functor_invoke.hpp"
#include "vm/module/member_function_invoke.hpp"
#include "vm/module/static_member_function_invoke.hpp"
#include "vm/object.hpp"
#include "vm/vm.hpp"

#include <cassert>
#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

namespace fetch {
namespace vm {

class Module
{
public:
  Module();
  ~Module() = default;

  template <typename Type>
  class ClassInterface
  {
  public:
    ClassInterface(Module *module__, TypeIndex type_index__)
      : module_(module__)
      , type_index_(type_index__)
    {}

    // default ctors
    template <typename ReturnType>
    ClassInterface &CreateConstructor(ReturnType (*constructor)(VM *, TypeId),
                                      ChargeAmount static_charge              = 1,
                                      ChargeAmount default_ctor_static_charge = 1)
    {
      return InternalCreateConstructor(constructor, static_charge, {})
          .CreateSerializeDefaultConstructor(constructor, default_ctor_static_charge);
    }

    template <typename Estimator, typename ReturnType>
    ClassInterface &CreateConstructor(ReturnType (*constructor)(VM *, TypeId),
                                      Estimator const &estimator,
                                      ChargeAmount     default_ctor_static_charge = 1)
    {
      return InternalCreateConstructor(constructor, 0, {estimator})
          .CreateSerializeDefaultConstructor(constructor, default_ctor_static_charge);
    }

    // non-default ctors
    template <typename ReturnType, typename Arg1, typename... Args>
    ClassInterface &CreateConstructor(ReturnType (*constructor)(VM *, TypeId, Arg1, Args...),
                                      ChargeAmount static_charge = 1)
    {
      return InternalCreateConstructor(constructor, static_charge, {});
    }

    template <typename Estimator, typename ReturnType, typename Arg1, typename... Args>
    ClassInterface &CreateConstructor(ReturnType (*constructor)(VM *, TypeId, Arg1, Args...),
                                      Estimator const &estimator)
    {
      return InternalCreateConstructor(constructor, 0, {estimator});
    }

    template <typename Constructor>
    ClassInterface &CreateSerializeDefaultConstructor(Constructor const &constructor,
                                                      ChargeAmount       static_charge = 1)
    {
      TypeIndex const type_index__ = type_index_;

      auto const estimator = [static_charge]() -> ChargeAmount { return static_charge; };

      DefaultConstructorHandler h = [constructor, estimator](VM *   vm,
                                                             TypeId type_id) -> Ptr<Object> {
        if (EstimateCharge(vm, ChargeEstimator<>{estimator}))
        {
          return constructor(vm, type_id);
        }

        return nullptr;
      };

      module_->deserialization_constructors_.insert({type_index__, std::move(h)});

      return *this;
    }

    template <typename ReturnType, typename... Args>
    ClassInterface &CreateStaticMemberFunction(std::string const &function_name,
                                               ReturnType (*f)(VM *, TypeId, Args...),
                                               ChargeAmount static_charge = 1)
    {
      return InternalCreateStaticMemberFunction(function_name, f, static_charge, {});
    }

    template <typename Estimator, typename ReturnType, typename... Args>
    ClassInterface &CreateStaticMemberFunction(std::string const &function_name,
                                               ReturnType (*f)(VM *, TypeId, Args...),
                                               Estimator const &estimator)
    {
      return InternalCreateStaticMemberFunction(function_name, f, 0, {estimator});
    }

    template <typename ReturnType, typename... Args>
    ClassInterface &CreateMemberFunction(std::string const &name, ReturnType (Type::*f)(Args...),
                                         ChargeAmount       static_charge = 1)
    {
      return InternalCreateMemberFunction<ReturnType, decltype(f), Args...>(name, f, static_charge,
                                                                            {});
    }

    template <typename ReturnType, typename... Args>
    ClassInterface &CreateMemberFunction(std::string const &name,
                                         ReturnType (Type::*f)(Args...) const,
                                         ChargeAmount static_charge = 1)
    {
      return InternalCreateMemberFunction<ReturnType, decltype(f), Args...>(name, f, static_charge,
                                                                            {});
    }

    template <typename Estimator, typename ReturnType, typename... Args>
    ClassInterface &CreateMemberFunction(std::string const &name, ReturnType (Type::*f)(Args...),
                                         Estimator const &  estimator)
    {
      return InternalCreateMemberFunction<ReturnType, decltype(f), Args...>(name, f, 0,
                                                                            {estimator});
    }

    template <typename Estimator, typename ReturnType, typename... Args>
    ClassInterface &CreateMemberFunction(std::string const &name,
                                         ReturnType (Type::*f)(Args...) const,
                                         Estimator const &estimator)
    {
      return InternalCreateMemberFunction<ReturnType, decltype(f), Args...>(name, f, 0,
                                                                            {estimator});
    }

    ClassInterface &EnableOperator(Operator op)
    {
      TypeIndex const type_index__            = type_index_;
      auto            compiler_setup_function = [type_index__, op](Compiler *compiler) {
        compiler->EnableOperator(type_index__, op);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);
      return *this;
    }

    template <typename GetterReturnType, typename... GetterArgs, typename SetterReturnType,
              typename... SetterArgs>
    ClassInterface &EnableIndexOperator(GetterReturnType (Type::*getter)(GetterArgs...),
                                        SetterReturnType (Type::*setter)(SetterArgs...),
                                        ChargeAmount static_getter_charge = 1,
                                        ChargeAmount static_setter_charge = 1)
    {
      return InternalEnableIndexOperator(getter, setter, static_getter_charge, static_setter_charge,
                                         {}, {});
    }

    template <typename GetterReturnType, typename... GetterArgs, typename SetterReturnType,
              typename... SetterArgs, typename GetterEstimator, typename SetterEstimator>
    ClassInterface &EnableIndexOperator(GetterReturnType (Type::*getter)(GetterArgs...),
                                        SetterReturnType (Type::*setter)(SetterArgs...),
                                        GetterEstimator const &getter_estimator,
                                        SetterEstimator const &setter_estimator)
    {
      return InternalEnableIndexOperator(getter, setter, 0, 0, {getter_estimator},
                                         {setter_estimator});
    }

    template <typename InstantiationType>
    ClassInterface &CreateInstantiationType()
    {
      TypeIndex const instantiation_type_index = TypeIndex(typeid(InstantiationType));
      TypeIndex const template_type_index      = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollTemplateParameters<InstantiationType>::Unroll(parameter_type_index_array);
      auto compiler_setup_function = [instantiation_type_index, template_type_index,
                                      parameter_type_index_array](Compiler *compiler) {
        compiler->CreateInstantiationType(instantiation_type_index, template_type_index,
                                          parameter_type_index_array);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);

      return *this;
    }

  private:
    template <typename ReturnType, typename... Args>
    ClassInterface &InternalCreateConstructor(ReturnType (*constructor)(VM *, TypeId, Args...),
                                              ChargeAmount                    static_charge,
                                              ChargeEstimator<Args...> const &estimator)
    {
      static_assert(IsPtr<ReturnType>::value, "Constructors must return a fetch::vm::Ptr");

      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollParameterTypes<Args...>::Unroll(parameter_type_index_array);

      Handler handler = [constructor, estimator](VM *vm) {
        InvokeConstructor(vm, vm->instruction_->type_id, constructor, estimator);
      };

      auto compiler_setup_function = [type_index__, parameter_type_index_array, handler,
                                      static_charge](Compiler *compiler) {
        compiler->CreateConstructor(type_index__, parameter_type_index_array, handler,
                                    static_charge);
      };

      module_->AddCompilerSetupFunction(compiler_setup_function);

      return *this;
    }

    template <typename ReturnType, typename... Args>
    ClassInterface &InternalCreateStaticMemberFunction(std::string const &function_name,
                                                       ReturnType (*f)(VM *, TypeId, Args...),
                                                       ChargeAmount static_charge,
                                                       ChargeEstimator<Args...> const &estimator)
    {
      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollParameterTypes<Args...>::Unroll(parameter_type_index_array);
      TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();

      Handler handler = [f, estimator](VM *vm) {
        InvokeStaticMemberFunction(vm, vm->instruction_->data, vm->instruction_->type_id, f,
                                   estimator);
      };

      auto compiler_setup_function = [type_index__, function_name, parameter_type_index_array,
                                      return_type_index, handler,
                                      static_charge](Compiler *compiler) {
        compiler->CreateStaticMemberFunction(type_index__, function_name,
                                             parameter_type_index_array, return_type_index, handler,
                                             static_charge);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);

      return *this;
    }

    template <typename GetterReturnType, typename... GetterArgs, typename SetterReturnType,
              typename... SetterArgs>
    ClassInterface &InternalEnableIndexOperator(
        GetterReturnType (Type::*getter)(GetterArgs...),
        SetterReturnType (Type::*setter)(SetterArgs...), ChargeAmount static_getter_charge,
        ChargeAmount static_setter_charge, ChargeEstimator<GetterArgs...> const &getter_estimator,
        ChargeEstimator<SetterArgs...> const &setter_estimator)
    {
      static_assert(sizeof...(GetterArgs) >= 1, "Getter should take at least one parameter");
      static_assert(sizeof...(SetterArgs) >= 2, "Setter should take at least two parameters");
      static_assert(
          std::is_same<
              GetterReturnType,
              std::decay_t<typename meta::GetLastType<std::tuple<SetterArgs...>>::type>>::value,
          "Inconsistent getter and setter definitions: getter return type should be the "
          "same as last setter parameter type (allowing for const and reference declarators)");

      TypeIndex const type_index__ = type_index_;

      TypeIndexArray setter_args_type_index_array;
      UnrollParameterTypes<SetterArgs...>::Unroll(setter_args_type_index_array);
      TypeIndex const output_type_index = setter_args_type_index_array.back();
      setter_args_type_index_array.pop_back();

      {  // sanity checks
        TypeIndexArray getter_args_type_index_array;
        UnrollParameterTypes<GetterArgs...>::Unroll(getter_args_type_index_array);
        assert(getter_args_type_index_array == setter_args_type_index_array);
        assert(output_type_index == TypeIndex(typeid(GetterReturnType)));
      }

      Handler get_handler = [getter, getter_estimator](VM *vm) {
        InvokeMemberFunction(vm, vm->instruction_->type_id, getter, getter_estimator);
      };
      Handler set_handler = [setter, setter_estimator](VM *vm) {
        InvokeMemberFunction(vm, vm->instruction_->type_id, setter, setter_estimator);
      };

      auto compiler_setup_function = [type_index__, setter_args_type_index_array, output_type_index,
                                      get_handler, set_handler, static_getter_charge,
                                      static_setter_charge](Compiler *compiler) {
        compiler->EnableIndexOperator(type_index__, setter_args_type_index_array, output_type_index,
                                      get_handler, set_handler, static_getter_charge,
                                      static_setter_charge);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);

      return *this;
    }

    template <typename ReturnType, typename MemberFunction, typename... Args>
    ClassInterface &InternalCreateMemberFunction(std::string const &             function_name,
                                                 MemberFunction const &          f,
                                                 ChargeAmount                    static_charge,
                                                 ChargeEstimator<Args...> const &estimator)
    {
      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollParameterTypes<Args...>::Unroll(parameter_type_index_array);
      TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();

      Handler handler = [f, estimator](VM *vm) {
        InvokeMemberFunction(vm, vm->instruction_->type_id, f, estimator);
      };

      auto compiler_setup_function = [type_index__, function_name, parameter_type_index_array,
                                      return_type_index, handler,
                                      static_charge](Compiler *compiler) {
        compiler->CreateMemberFunction(type_index__, function_name, parameter_type_index_array,
                                       return_type_index, handler, static_charge);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);

      return *this;
    }

    Module *  module_;
    TypeIndex type_index_;
  };

  template <typename ReturnType, typename... Args>
  void CreateFreeFunction(std::string const &name, ReturnType (*f)(VM *, Args...),
                          ChargeAmount       static_charge = 1)
  {
    InternalCreateFreeFunction(name, f, static_charge, {});
  }

  template <typename Estimator, typename ReturnType, typename... Args>
  void CreateFreeFunction(std::string const &name, ReturnType (*f)(VM *, Args...),
                          Estimator const &  estimator)
  {
    InternalCreateFreeFunction(name, f, 0, {estimator});
  }

  template <typename Functor>
  void CreateFreeFunction(std::string const &name, Functor const &functor,
                          ChargeAmount static_charge = 1)
  {
    using HandlerArgs = typename FunctorTraits<Functor>::args_tuple_type;
    // Remove leading VM* argument
    using EstimatorArgsTuple = typename meta::RemoveFirstType<HandlerArgs>::type;
    using EstimatorType      = typename meta::UnpackTuple<EstimatorArgsTuple, ChargeEstimator>;

    InternalCreateFreeFunctor(name, functor, static_charge, EstimatorType{});
  }

  template <typename Estimator, typename Functor>
  void CreateFreeFunction(std::string const &name, Functor const &functor,
                          Estimator const &estimator)
  {
    InternalCreateFreeFunctor(name, functor, 0, estimator);
  }

  template <typename Type>
  ClassInterface<Type> CreateClassType(std::string const &name)
  {
    TypeIndex const type_index              = TypeIndex(typeid(Type));
    auto            compiler_setup_function = [name, type_index](Compiler *compiler) {
      compiler->CreateClassType(name, type_index);
    };
    AddCompilerSetupFunction(compiler_setup_function);
    return ClassInterface<Type>(this, type_index);
  }

  template <typename Type>
  ClassInterface<Type> GetClassInterface()
  {
    TypeIndex const type_index = TypeIndex(typeid(Type));
    return ClassInterface<Type>(this, type_index);
  }

private:
  template <typename ReturnType, typename... Args>
  void InternalCreateFreeFunction(std::string const &name, ReturnType (*f)(VM *, Args...),
                                  ChargeAmount       static_charge,
                                  ChargeEstimator<Args...> const &estimator)
  {
    TypeIndexArray parameter_type_index_array;
    UnrollParameterTypes<Args...>::Unroll(parameter_type_index_array);
    TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();

    Handler handler = [f, estimator](VM *vm) {
      InvokeFreeFunction(vm, vm->instruction_->type_id, f, estimator);
    };

    auto compiler_setup_function = [name, parameter_type_index_array, return_type_index, handler,
                                    static_charge](Compiler *compiler) {
      compiler->CreateFreeFunction(name, parameter_type_index_array, return_type_index, handler,
                                   static_charge);
    };
    AddCompilerSetupFunction(compiler_setup_function);
  }

  template <typename Estimator, typename Functor>
  void InternalCreateFreeFunctor(std::string const &name, Functor const &functor,
                                 ChargeAmount static_charge, Estimator const &estimator)
  {
    using ReturnType     = typename FunctorTraits<Functor>::return_type;
    using SignatureTuple = typename FunctorTraits<Functor>::args_tuple_type;
    using ParameterTuple = typename meta::RemoveFirstType<SignatureTuple>::type;
    TypeIndexArray parameter_type_index_array;
    UnrollTupleParameterTypes<ParameterTuple>::Unroll(parameter_type_index_array);
    TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();

    Handler handler = [functor, estimator](VM *vm) {
      InvokeFunctor(vm, vm->instruction_->type_id, functor, estimator, ParameterTuple());
    };

    auto compiler_setup_function = [name, parameter_type_index_array, return_type_index, handler,
                                    static_charge](Compiler *compiler) {
      compiler->CreateFreeFunction(name, parameter_type_index_array, return_type_index, handler,
                                   static_charge);
    };
    AddCompilerSetupFunction(compiler_setup_function);
  }

  void CompilerSetup(Compiler *compiler)
  {
    for (auto const &compiler_setup_function : compiler_setup_functions_)
    {
      compiler_setup_function(compiler);
    }
    compiler->GetDetails(type_info_array_, type_info_map_, registered_types_, function_info_array_);
  }

  void GetDetails(TypeInfoArray &type_info_array, TypeInfoMap &type_info_map,
                  RegisteredTypes &registered_types, FunctionInfoArray &function_info_array,
                  DeserializeConstructorMap &deserialization_constructors)
  {
    type_info_array              = type_info_array_;
    type_info_map                = type_info_map_;
    registered_types             = registered_types_;
    function_info_array          = function_info_array_;
    deserialization_constructors = deserialization_constructors_;
  }

  using CompilerSetupFunction = std::function<void(Compiler *)>;

  void AddCompilerSetupFunction(CompilerSetupFunction const &function)
  {
    compiler_setup_functions_.push_back(function);
  }

  std::vector<CompilerSetupFunction> compiler_setup_functions_;
  TypeInfoArray                      type_info_array_;
  TypeInfoMap                        type_info_map_;
  RegisteredTypes                    registered_types_;
  FunctionInfoArray                  function_info_array_;
  DeserializeConstructorMap          deserialization_constructors_;

  friend class Compiler;
  friend class VM;
};

}  // namespace vm
}  // namespace fetch
