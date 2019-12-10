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

#include "core/macros.hpp"
#include "meta/callable/callable_traits.hpp"
#include "meta/tuple.hpp"
#include "vm/compiler.hpp"
#include "vm/estimate_charge.hpp"
#include "vm/module/base.hpp"
#include "vm/module/binding_interfaces.hpp"
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
      return InternalCreateConstructor(constructor, static_charge, ChargeEstimator<>{})
          .CreateSerializeDefaultConstructor(constructor, default_ctor_static_charge);
    }

    template <typename Estimator, typename ReturnType>
    ClassInterface &CreateConstructor(ReturnType (*constructor)(VM *, TypeId), Estimator estimator,
                                      ChargeAmount default_ctor_static_charge = 1)
    {
      return InternalCreateConstructor(constructor, 0, ChargeEstimator<>{estimator})
          .CreateSerializeDefaultConstructor(constructor, default_ctor_static_charge);
    }

    // non-default ctors
    template <typename ReturnType, typename Arg1, typename... Args>
    ClassInterface &CreateConstructor(ReturnType (*constructor)(VM *, TypeId, Arg1, Args...),
                                      ChargeAmount static_charge = 1)
    {
      return InternalCreateConstructor(constructor, static_charge,
                                       ChargeEstimator<Arg1, Args...>{});
    }

    template <typename Estimator, typename ReturnType, typename Arg1, typename... Args>
    ClassInterface &CreateConstructor(ReturnType (*constructor)(VM *, TypeId, Arg1, Args...),
                                      Estimator estimator)
    {
      return InternalCreateConstructor(constructor, 0, ChargeEstimator<Arg1, Args...>{estimator});
    }

    template <typename Constructor>
    ClassInterface &CreateSerializeDefaultConstructor(Constructor  constructor,
                                                      ChargeAmount static_charge = 1)
    {
      using ReturnType = typename meta::CallableTraits<Constructor>::ReturnType;
      using Params     = typename meta::CallableTraits<Constructor>::ArgsTupleType;
      static_assert(IsPtr<ReturnType>::value, "Constructors must return a fetch::vm::Ptr");
      static_assert(std::is_same<Params, std::tuple<VM *, TypeId>>::value,
                    "Invalid default constructor handler: argument types should be (VM*, TypeId)");

      TypeIndex const type_index__ = type_index_;

      auto const estimator = [static_charge]() -> ChargeAmount { return static_charge; };

      DefaultConstructorHandler h = [constructor, estimator](VM *   vm,
                                                             TypeId type_id) -> Ptr<Object> {
        if (EstimateCharge(vm, ChargeEstimator<>{std::move(estimator)}, std::tuple<>{}))
        {
          return constructor(vm, type_id);
        }

        return {};
      };

      module_->deserialization_constructors_.insert({type_index__, std::move(h)});

      return *this;
    }

    template <typename T>
    using CPPCopyConstructor = std::function<Ptr<Object>(vm::VM *, TypeId, T const &)>;

    template <typename CPPType>
    ClassInterface &CreateCPPCopyConstructor(CPPCopyConstructor<CPPType> constructor,
                                             ChargeAmount                static_charge = 1)
    {
      // Note that unlike all the other functions, we need to be able to do the look up in the
      // native C++ type and not the VM type.
      TypeIndex const type_index = TypeIndex(typeid(CPPType));

      auto const estimator = [static_charge]() -> ChargeAmount { return static_charge; };

      CPPCopyConstructorHandler h = [constructor, estimator](VM *        vm,
                                                             void const *ptr) -> Ptr<Object> {
        if (EstimateCharge(vm, ChargeEstimator<>{std::move(estimator)}, std::tuple<>{}))
        {
          auto type_id   = vm->GetTypeId<Type>();
          auto typed_ptr = static_cast<CPPType const *>(ptr);
          return constructor(vm, type_id, *typed_ptr);
        }

        return {};
      };
      module_->cpp_copy_constructors_.insert({type_index, std::move(h)});

      return *this;
    }

    template <typename Callable>
    ClassInterface &CreateStaticMemberFunction(std::string const &name, Callable callable,
                                               ChargeAmount static_charge = 1)
    {
      using Traits        = typename meta::CallableTraits<Callable>;
      using Params        = typename Traits::ArgsTupleType;
      using EtchParams    = typename meta::Tuple<Params>::template DropInitial<2>::type;
      using EstimatorType = meta::UnpackTuple<EtchParams, ChargeEstimator>;

      return InternalCreateStaticMemberFunction(name, callable, static_charge, EstimatorType{});
    }

    template <typename Estimator, typename Callable>
    ClassInterface &CreateStaticMemberFunction(std::string const &name, Callable callable,
                                               Estimator estimator)
    {
      return InternalCreateStaticMemberFunction(name, callable, 0, estimator);
    }

    template <typename Callable>
    ClassInterface &CreateMemberFunction(std::string const &name, Callable callable,
                                         ChargeAmount static_charge = 1)
    {
      using Traits     = typename meta::CallableTraits<Callable>;
      using EtchParams = typename Traits::ArgsTupleType;

      using EstimatorArgs =
          typename meta::Tuple<EtchParams>::template Prepend<std::tuple<Ptr<Type> const &>>::type;

      using EstimatorType = meta::UnpackTuple<EstimatorArgs, ChargeEstimator>;

      return InternalCreateMemberFunction(name, callable, static_charge, EstimatorType{});
    }

    template <typename Estimator, typename Callable>
    ClassInterface &CreateMemberFunction(std::string const &name, Callable callable,
                                         Estimator estimator)
    {
      return InternalCreateMemberFunction(name, callable, 0, estimator);
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

    ClassInterface &EnableLeftOperator(Operator op)
    {
      TypeIndex const type_index__            = type_index_;
      auto            compiler_setup_function = [type_index__, op](Compiler *compiler) {
        compiler->EnableLeftOperator(type_index__, op);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);
      return *this;
    }

    ClassInterface &EnableRightOperator(Operator op)
    {
      TypeIndex const type_index__            = type_index_;
      auto            compiler_setup_function = [type_index__, op](Compiler *compiler) {
        compiler->EnableRightOperator(type_index__, op);
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
                                         ChargeEstimator<Ptr<Type> const &, GetterArgs...>{},
                                         ChargeEstimator<Ptr<Type> const &, SetterArgs...>{});
    }

    template <typename GetterReturnType, typename... GetterArgs, typename SetterReturnType,
              typename... SetterArgs, typename GetterEstimator, typename SetterEstimator>
    ClassInterface &EnableIndexOperator(GetterReturnType (Type::*getter)(GetterArgs...),
                                        SetterReturnType (Type::*setter)(SetterArgs...),
                                        GetterEstimator getter_estimator,
                                        SetterEstimator setter_estimator)
    {
      return InternalEnableIndexOperator(
          getter, setter, 0, 0, ChargeEstimator<Ptr<Type> const &, GetterArgs...>{getter_estimator},
          ChargeEstimator<Ptr<Type> const &, SetterArgs...>{setter_estimator});
    }

    template <typename InstantiationType>
    ClassInterface &CreateInstantiationType()
    {
      TypeIndex const instantiation_type_index = TypeIndex(typeid(InstantiationType));
      TypeIndex const template_type_index      = type_index_;
      TypeIndexArray  template_parameter_type_index_array;
      UnrollTemplateParameters<InstantiationType>::Unroll(template_parameter_type_index_array);
      auto compiler_setup_function = [instantiation_type_index, template_type_index,
                                      template_parameter_type_index_array](Compiler *compiler) {
        compiler->CreateTemplateInstantiationType(instantiation_type_index, template_type_index,
                                                  template_parameter_type_index_array);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);

      return *this;
    }

  private:
    template <typename Estimator, typename Callable>
    ClassInterface &InternalCreateConstructor(Callable callable, ChargeAmount static_charge,
                                              Estimator estimator)
    {
      using Traits      = typename meta::CallableTraits<Callable>;
      using ReturnType  = typename Traits::ReturnType;
      using Params      = typename Traits::ArgsTupleType;
      using EtchParams  = typename meta::Tuple<Params>::template DropInitial<2>::type;
      using ExtraParams = typename meta::Tuple<Params>::template TakeInitial<2>::type;
      static_assert(IsPtr<ReturnType>::value, "Constructors must return a fetch::vm::Ptr");
      static_assert(std::is_same<ExtraParams, std::tuple<VM *, TypeId>>::value,
                    "Invalid constructor handler: initial two arguments should be (VM*, TypeId)");

      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollTupleParameterTypes<EtchParams>::Unroll(parameter_type_index_array);

      Handler handler = [callable, estimator](VM *vm) {
        Constructor<EtchParams>::InvokeHandler(vm, estimator, callable, vm,
                                               vm->instruction_->type_id);
      };

      auto compiler_setup_function = [type_index__, parameter_type_index_array, handler,
                                      static_charge](Compiler *compiler) {
        compiler->CreateConstructor(type_index__, parameter_type_index_array, handler,
                                    static_charge);
      };

      module_->AddCompilerSetupFunction(compiler_setup_function);

      return *this;
    }

    template <typename Estimator, typename Callable>
    ClassInterface &InternalCreateStaticMemberFunction(std::string const &name, Callable callable,
                                                       ChargeAmount static_charge,
                                                       Estimator    estimator)
    {
      using Traits      = typename meta::CallableTraits<Callable>;
      using Params      = typename Traits::ArgsTupleType;
      using EtchParams  = typename meta::Tuple<Params>::template DropInitial<2>::type;
      using ExtraParams = typename meta::Tuple<Params>::template TakeInitial<2>::type;
      using ReturnType  = typename Traits::ReturnType;
      static_assert(
          std::is_same<ExtraParams, std::tuple<VM *, TypeId>>::value,
          "Invalid static member function handler: initial two arguments should be (VM*, TypeId)");

      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollTupleParameterTypes<EtchParams>::Unroll(parameter_type_index_array);
      TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();

      Handler handler = [callable, estimator](VM *vm) {
        StaticMemberFunction<EtchParams>::InvokeHandler(vm, estimator, callable, vm,
                                                        vm->instruction_->data);
      };
      auto compiler_setup_function = [type_index__, name, parameter_type_index_array,
                                      return_type_index, handler,
                                      static_charge](Compiler *compiler) {
        compiler->CreateStaticMemberFunction(type_index__, name, parameter_type_index_array,
                                             return_type_index, handler, static_charge);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);
      return *this;
    }

    template <typename Getter, typename Setter, typename GetterEstimator, typename SetterEstimator>
    ClassInterface &InternalEnableIndexOperator(Getter getter, Setter setter,
                                                ChargeAmount    static_getter_charge,
                                                ChargeAmount    static_setter_charge,
                                                GetterEstimator getter_estimator,
                                                SetterEstimator setter_estimator)
    {
      using GetterTraits     = meta::CallableTraits<Getter>;
      using SetterTraits     = meta::CallableTraits<Setter>;
      using GetterEtchParams = typename GetterTraits::ArgsTupleType;
      using SetterEtchParams = typename SetterTraits::ArgsTupleType;
      using GetterReturnType = typename GetterTraits::ReturnType;
      static_assert(GetterTraits::arg_count() >= 1, "Getter should take at least one parameter");
      static_assert(SetterTraits::arg_count() >= 2, "Setter should take at least two parameters");
      static_assert(
          std::is_same<GetterReturnType,
                       std::decay_t<typename meta::Tuple<SetterEtchParams>::LastType>>::value,
          "Inconsistent getter and setter definitions: getter return type should be the "
          "same as last setter parameter type (allowing for const and reference declarators)");
      static_assert(SetterTraits::is_void(), "Expected setter handler to return void");

      TypeIndex const type_index__ = type_index_;

      TypeIndexArray setter_args_type_index_array;
      UnrollTupleParameterTypes<SetterEtchParams>::Unroll(setter_args_type_index_array);
      TypeIndex const output_type_index = setter_args_type_index_array.back();
      setter_args_type_index_array.pop_back();

      {  // sanity checks
        TypeIndexArray getter_args_type_index_array;
        UnrollTupleParameterTypes<GetterEtchParams>::Unroll(getter_args_type_index_array);
        assert(getter_args_type_index_array == setter_args_type_index_array);
        FETCH_UNUSED_ALIAS(GetterReturnType);
        assert(output_type_index == TypeIndex(typeid(GetterReturnType)));
      }

      Handler get_handler = [getter, getter_estimator](VM *vm) {
        MemberFunction<GetterEtchParams>::InvokeHandler(vm, getter_estimator, getter);
      };
      Handler set_handler = [setter, setter_estimator](VM *vm) {
        MemberFunction<SetterEtchParams>::InvokeHandler(vm, setter_estimator, setter);
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

    template <typename Estimator, typename Callable>
    ClassInterface &InternalCreateMemberFunction(std::string const &name, Callable callable,
                                                 ChargeAmount static_charge, Estimator estimator)
    {
      using EtchParams = typename meta::CallableTraits<Callable>::ArgsTupleType;
      using ReturnType = typename meta::CallableTraits<Callable>::ReturnType;

      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollTupleParameterTypes<EtchParams>::Unroll(parameter_type_index_array);
      TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();

      Handler handler = [callable, estimator](VM *vm) {
        MemberFunction<EtchParams>::InvokeHandler(vm, estimator, callable);
      };

      auto compiler_setup_function = [type_index__, name, parameter_type_index_array,
                                      return_type_index, handler,
                                      static_charge](Compiler *compiler) {
        compiler->CreateMemberFunction(type_index__, name, parameter_type_index_array,
                                       return_type_index, handler, static_charge);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);

      return *this;
    }

    Module *  module_;
    TypeIndex type_index_;
  };

  template <typename Callable>
  void CreateFreeFunction(std::string const &name, Callable callable,
                          ChargeAmount static_charge = 1)
  {
    using Traits        = typename meta::CallableTraits<Callable>;
    using Params        = typename Traits::ArgsTupleType;
    using EtchParams    = typename meta::Tuple<Params>::template DropInitial<1>::type;
    using EstimatorType = meta::UnpackTuple<EtchParams, ChargeEstimator>;

    InternalCreateFreeFunction(name, callable, static_charge, EstimatorType{});
  }

  template <typename Estimator, typename Callable>
  void CreateFreeFunction(std::string const &name, Callable callable, Estimator estimator)
  {
    InternalCreateFreeFunction(name, callable, 0, estimator);
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

  template <typename Type, typename... Args>
  ClassInterface<Type> CreateTemplateType(std::string const &name)
  {
    TypeIndexArray allowed_types_index_array;
    UnrollTypes<Args...>::Unroll(allowed_types_index_array);
    TypeIndex const type_index              = TypeIndex(typeid(Type));
    auto            compiler_setup_function = [name, type_index,
                                    allowed_types_index_array](Compiler *compiler) {
      compiler->CreateTemplateType(name, type_index, allowed_types_index_array);
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

  RegisteredTypes const &registered_types() const
  {
    return registered_types_;
  }

  const TypeInfoArray &GetTypeInfoArray() const
  {
    return type_info_array_;
  }
  const DeserializeConstructorMap &GetDeserializationConstructors() const
  {
    return deserialization_constructors_;
  }

private:
  template <typename Estimator, typename Callable>
  void InternalCreateFreeFunction(std::string const &name, Callable callable,
                                  ChargeAmount static_charge, Estimator estimator)
  {
    using Traits      = typename meta::CallableTraits<Callable>;
    using ReturnType  = typename Traits::ReturnType;
    using Params      = typename Traits::ArgsTupleType;
    using EtchParams  = typename meta::Tuple<Params>::template DropInitial<1>::type;
    using ExtraParams = typename meta::Tuple<Params>::template TakeInitial<1>::type;
    static_assert(std::is_same<ExtraParams, std::tuple<VM *>>::value,
                  "Invalid free function handler: initial argument should be VM*");

    TypeIndexArray parameter_type_index_array;
    UnrollTupleParameterTypes<EtchParams>::Unroll(parameter_type_index_array);
    TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();

    Handler handler = [callable, estimator](VM *vm) {
      using EstimatorType = meta::UnpackTuple<EtchParams, ChargeEstimator>;
      FreeFunction<EtchParams>::InvokeHandler(vm, EstimatorType{estimator}, callable, vm);
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
                  DeserializeConstructorMap &deserialization_constructors,
                  CPPCopyConstructorMap &    cpp_copy_constructors) const
  {
    type_info_array              = type_info_array_;
    type_info_map                = type_info_map_;
    registered_types             = registered_types_;
    function_info_array          = function_info_array_;
    deserialization_constructors = deserialization_constructors_;
    cpp_copy_constructors        = cpp_copy_constructors_;
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

  // C++ copy constructors are only used for easy contruction
  // of C++ objects as Etch objects
  CPPCopyConstructorMap cpp_copy_constructors_;

  friend class Compiler;
  friend class VM;
};

}  // namespace vm
}  // namespace fetch
