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

#include "vm/address.hpp"
#include "vm/array.hpp"
#include "vm/compiler.hpp"
#include "vm/map.hpp"
#include "vm/matrix.hpp"
#include "vm/module/base.hpp"
#include "vm/module/constructor_invoke.hpp"
#include "vm/module/free_function_invoke.hpp"
#include "vm/module/functor_invoke.hpp"
#include "vm/module/member_function_invoke.hpp"
#include "vm/module/static_member_function_invoke.hpp"
#include "vm/state.hpp"
#include "vm/vm.hpp"

#include <functional>

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

    template <typename... Ts>
    ClassInterface &CreateConstuctor()
    {
      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollTypes<Ts...>::Unroll(parameter_type_index_array);
      Handler handler = [](VM *vm) {
        InvokeConstructor<Type, Ts...>(vm, vm->instruction_->type_id);
      };
      auto compiler_setup_function = [type_index__, parameter_type_index_array,
                                      handler](Compiler *compiler) {
        compiler->CreateConstructor(type_index__, parameter_type_index_array, handler);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);
      return *this;
    }

    template <typename ReturnType, typename... Ts>
    ClassInterface &CreateStaticMemberFunction(std::string const &function_name,
                                               ReturnType (*f)(VM *, TypeId, Ts...))
    {
      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollParameterTypes<Ts...>::Unroll(parameter_type_index_array);
      TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();
      Handler         handler           = [f](VM *vm) {
        InvokeStaticMemberFunction(vm, vm->instruction_->data, vm->instruction_->type_id, f);
      };
      auto compiler_setup_function = [type_index__, function_name, parameter_type_index_array,
                                      return_type_index, handler](Compiler *compiler) {
        compiler->CreateStaticMemberFunction(
            type_index__, function_name, parameter_type_index_array, return_type_index, handler);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);
      return *this;
    }

    template <typename ReturnType, typename... Ts>
    ClassInterface &CreateMemberFunction(std::string const &name, ReturnType (Type::*f)(Ts...))
    {
      using MemberFunction = ReturnType (Type::*)(Ts...);
      return InternalCreateMemberFunction<ReturnType, MemberFunction, Ts...>(name, f);
    }

    template <typename ReturnType, typename... Ts>
    ClassInterface &CreateMemberFunction(std::string const &name,
                                         ReturnType (Type::*f)(Ts...) const)
    {
      using MemberFunction = ReturnType (Type::*)(Ts...) const;
      return InternalCreateMemberFunction<ReturnType, MemberFunction, Ts...>(name, f);
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

    // input type 1, input type 2 ... input type N, output type
    template <typename... Types>
    ClassInterface &EnableIndexOperator()  // order changed!
    {
      static_assert(sizeof...(Types) >= 2, "2 or more types expected");
      using Tuple       = std::tuple<Types...>;
      using InputsTuple = typename RemoveLastType<Tuple>::type;
      using OutputType  = typename GetLastType<Tuple>::type;
      using Getter      = typename IndexedValueGetter<Type, InputsTuple, OutputType>::type;
      using Setter      = typename IndexedValueSetter<Type, InputsTuple, OutputType>::type;
      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  input_type_index_array;
      UnrollTypes<Types...>::Unroll(input_type_index_array);
      TypeIndex const output_type_index = input_type_index_array.back();
      input_type_index_array.pop_back();
      Getter  gf          = &Type::GetIndexedValue;
      Handler get_handler = [gf](VM *vm) {
        InvokeMemberFunction(vm, vm->instruction_->type_id, gf);
      };
      Setter  sf          = &Type::SetIndexedValue;
      Handler set_handler = [sf](VM *vm) {
        InvokeMemberFunction(vm, vm->instruction_->type_id, sf);
      };
      auto compiler_setup_function = [type_index__, input_type_index_array, output_type_index,
                                      get_handler, set_handler](Compiler *compiler) {
        compiler->EnableIndexOperator(type_index__, input_type_index_array, output_type_index,
                                      get_handler, set_handler);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);
      return *this;
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
    template <typename ReturnType, typename MemberFunction, typename... Ts>
    ClassInterface &InternalCreateMemberFunction(std::string const &function_name, MemberFunction f)
    {
      TypeIndex const type_index__ = type_index_;
      TypeIndexArray  parameter_type_index_array;
      UnrollParameterTypes<Ts...>::Unroll(parameter_type_index_array);
      TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();
      Handler handler = [f](VM *vm) { InvokeMemberFunction(vm, vm->instruction_->type_id, f); };
      auto    compiler_setup_function = [type_index__, function_name, parameter_type_index_array,
                                      return_type_index, handler](Compiler *compiler) {
        compiler->CreateMemberFunction(type_index__, function_name, parameter_type_index_array,
                                       return_type_index, handler);
      };
      module_->AddCompilerSetupFunction(compiler_setup_function);
      return *this;
    }

    Module *  module_;
    TypeIndex type_index_;
  };

  template <typename ReturnType>
  void CreateFreeFunctionFromLambda(std::string const &name, std::function<ReturnType(VM *)> f)
  {
    TypeIndexArray  parameter_type_index_array;
    TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();
    Handler         handler           = [f](VM *vm) {
      ReturnType           result               = f(vm);
      constexpr auto const stack_pointer_offset = -1;
      StackSetter<ReturnType>::Set(vm, stack_pointer_offset, std::move(result),
                                   vm->instruction_->type_id);
      vm->sp_ -= stack_pointer_offset;
    };

    auto compiler_setup_function = [name, parameter_type_index_array, return_type_index,
                                    handler](Compiler *compiler) {
      compiler->CreateFreeFunction(name, parameter_type_index_array, return_type_index, handler);
    };
    AddCompilerSetupFunction(compiler_setup_function);
  }

  template <typename ReturnType, typename... Ts>
  void CreateFreeFunction(std::string const &name, ReturnType (*f)(VM *, Ts...))
  {
    TypeIndexArray parameter_type_index_array;
    UnrollParameterTypes<Ts...>::Unroll(parameter_type_index_array);
    TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();
    Handler         handler = [f](VM *vm) { InvokeFreeFunction(vm, vm->instruction_->type_id, f); };
    auto            compiler_setup_function = [name, parameter_type_index_array, return_type_index,
                                    handler](Compiler *compiler) {
      compiler->CreateFreeFunction(name, parameter_type_index_array, return_type_index, handler);
    };
    AddCompilerSetupFunction(compiler_setup_function);
  }

  /*
  // Create a free function that takes an Int32 and a Float32 and returns a Float64
  auto lambda = [](fetch::vm::VM*, int32_t i, float f)
  {
    return double(float(i) + f);
  };
  module.CreateFreeFunction("myfunc", lambda);
  */
  template <typename Functor>
  void CreateFreeFunction(std::string const &name, Functor const &functor)
  {
    using ReturnType     = typename FunctorReturnTypeExtractor<Functor>::type;
    using SignatureTuple = typename FunctorSignatureExtractor<Functor>::type;
    using ParameterTuple = typename RemoveFirstType<SignatureTuple>::type;
    TypeIndexArray parameter_type_index_array;
    UnrollTupleParameterTypes<ParameterTuple>::Unroll(parameter_type_index_array);
    TypeIndex const return_type_index = TypeGetter<ReturnType>::GetTypeIndex();

    Handler handler = [functor](VM *vm) {
      InvokeFunctor(vm, vm->instruction_->type_id, functor, ParameterTuple());
    };
    auto compiler_setup_function = [name, parameter_type_index_array, return_type_index,
                                    handler](Compiler *compiler) {
      compiler->CreateFreeFunction(name, parameter_type_index_array, return_type_index, handler);
    };
    AddCompilerSetupFunction(compiler_setup_function);
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
  void CompilerSetup(Compiler *compiler)
  {
    for (auto const &compiler_setup_function : compiler_setup_functions_)
    {
      compiler_setup_function(compiler);
    }
    compiler->GetDetails(type_info_array_, type_info_map_, registered_types_, function_info_array_);
  }

  void GetDetails(TypeInfoArray &type_info_array, TypeInfoMap &type_info_map,
                  RegisteredTypes &registered_types, FunctionInfoArray &function_info_array)
  {
    type_info_array     = type_info_array_;
    type_info_map       = type_info_map_;
    registered_types    = registered_types_;
    function_info_array = function_info_array_;
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

  friend class Compiler;
  friend class VM;
};

}  // namespace vm
}  // namespace fetch
