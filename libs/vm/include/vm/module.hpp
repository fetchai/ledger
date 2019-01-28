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

#include "vm/array.hpp"
#include "vm/compiler.hpp"
#include "vm/map.hpp"
#include "vm/matrix.hpp"
#include "vm/module/base.hpp"
#include "vm/module/free_function_invoke.hpp"
#include "vm/module/instance_function_invoke.hpp"
#include "vm/module/type_constructor_invoke.hpp"
#include "vm/module/type_function_invoke.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm {

class Module
{
public:
  Module();
  ~Module() = default;

  template <typename ObjectType>
  struct ClassInterface
  {
    Module *module;
    TypeId  type_id;

    ClassInterface(Module *module__, TypeId type_id__)
    {
      module  = module__;
      type_id = type_id__;
    }

    template <typename... Ts>
    ClassInterface &CreateTypeConstuctor()
    {
      TypeId         type_id__ = type_id;
      Opcode         opcode    = module->GetNextOpcode();
      TypeIndexArray type_index_array;
      UnrollTypes<Ts...>::Unroll(type_index_array);
      TypeIdArray type_id_array           = module->GetTypeIds(type_index_array);
      auto        compiler_setup_function = [type_id__, opcode, type_id_array](Compiler *compiler) {
        compiler->CreateOpcodeTypeConstructor(type_id__, opcode, type_id_array);
      };
      module->AddCompilerSetupFunction(compiler_setup_function);
      OpcodeHandler handler = [](VM *vm) {
        InvokeTypeConstructor<ObjectType, Ts...>(vm, vm->instruction_->type_id);
      };
      module->AddOpcodeHandler(opcode, handler);
      return *this;
    }

    template <typename ReturnType, typename... Ts>
    ClassInterface &CreateTypeFunction(std::string const &name,
                                       ReturnType (*f)(VM *, TypeId, Ts...))
    {
      TypeId         type_id__ = type_id;
      Opcode         opcode    = module->GetNextOpcode();
      TypeIndexArray type_index_array;
      UnrollParameterTypes<Ts...>::Unroll(type_index_array);
      TypeIdArray type_id_array  = module->GetTypeIds(type_index_array);
      TypeId      return_type_id = module->GetTypeId(TypeGetter<ReturnType>::GetTypeIndex());
      auto        compiler_setup_function = [type_id__, name, opcode, type_id_array,
                                      return_type_id](Compiler *compiler) {
        compiler->CreateOpcodeTypeFunction(type_id__, name, opcode, type_id_array, return_type_id);
      };
      module->AddCompilerSetupFunction(compiler_setup_function);
      OpcodeHandler handler = [f](VM *vm) {
        InvokeTypeFunction(vm, vm->instruction_->data.ui16, vm->instruction_->type_id, f);
      };
      module->AddOpcodeHandler(opcode, handler);
      return *this;
    }

    template <typename ReturnType, typename... Ts>
    ClassInterface &CreateInstanceFunction(std::string const &name,
                                           ReturnType (ObjectType::*f)(Ts...))
    {
      TypeId         type_id__ = type_id;
      Opcode         opcode    = module->GetNextOpcode();
      TypeIndexArray type_index_array;
      UnrollParameterTypes<Ts...>::Unroll(type_index_array);
      TypeIdArray type_id_array  = module->GetTypeIds(type_index_array);
      TypeId      return_type_id = module->GetTypeId(TypeGetter<ReturnType>::GetTypeIndex());
      auto        compiler_setup_function = [type_id__, name, opcode, type_id_array,
                                      return_type_id](Compiler *compiler) {
        compiler->CreateOpcodeInstanceFunction(type_id__, name, opcode, type_id_array,
                                               return_type_id);
      };
      module->AddCompilerSetupFunction(compiler_setup_function);
      OpcodeHandler handler = [f](VM *vm) {
        InvokeInstanceFunction(vm, vm->instruction_->type_id, f);
      };
      module->AddOpcodeHandler(opcode, handler);
      return *this;
    }
  };

  template <typename ReturnType, typename... Ts>
  void CreateFreeFunction(std::string const &name, ReturnType (*f)(VM *, Ts...))
  {
    Opcode         opcode = GetNextOpcode();
    TypeIndexArray type_index_array;
    UnrollParameterTypes<Ts...>::Unroll(type_index_array);
    TypeIdArray type_id_array           = GetTypeIds(type_index_array);
    TypeId      return_type_id          = GetTypeId(TypeGetter<ReturnType>::GetTypeIndex());
    auto        compiler_setup_function = [name, opcode, type_id_array,
                                    return_type_id](Compiler *compiler) {
      compiler->CreateOpcodeFreeFunction(name, opcode, type_id_array, return_type_id);
    };
    AddCompilerSetupFunction(compiler_setup_function);
    OpcodeHandler handler = [f](VM *vm) { InvokeFreeFunction(vm, vm->instruction_->type_id, f); };
    AddOpcodeHandler(opcode, handler);
  }

  template <typename ObjectType>
  ClassInterface<ObjectType> CreateClassType(std::string const &name)
  {
    TypeId type_id = GetNextTypeId();
    RegisterType(TypeIndex(typeid(ObjectType)), type_id);
    auto compiler_setup_function = [name, type_id](Compiler *compiler) {
      compiler->CreateClassType(name, type_id);
    };
    AddCompilerSetupFunction(compiler_setup_function);
    return ClassInterface<ObjectType>(this, type_id);
  }

  template <template <class, class...> class Template, typename T, typename... Ts>
  TypeId CreateTemplateInstantiationType(TypeId template_type_id)
  {
    using InstantiationType = Template<T, Ts...>;
    TypeId type_id          = GetNextTypeId();
    RegisterType(TypeIndex(typeid(InstantiationType)), type_id);
    TypeIndexArray type_index_array;
    UnrollTypes<T, Ts...>::Unroll(type_index_array);
    TypeIdArray type_id_array    = GetTypeIds(type_index_array);
    auto compiler_setup_function = [type_id, template_type_id, type_id_array](Compiler *compiler) {
      compiler->CreateTemplateInstantiationType(type_id, template_type_id, type_id_array);
    };
    AddCompilerSetupFunction(compiler_setup_function);
    return type_id;
  }

private:
  template <typename ObjectType>
  ClassInterface<ObjectType> RegisterClassType(TypeId type_id)
  {
    RegisterType(TypeIndex(typeid(ObjectType)), type_id);
    return ClassInterface<ObjectType>(this, type_id);
  }

  template <typename ObjectType>
  ClassInterface<ObjectType> RegisterTemplateType(TypeId type_id)
  {
    RegisterType(TypeIndex(typeid(ObjectType)), type_id);
    return ClassInterface<ObjectType>(this, type_id);
  }

  void CompilerSetup(Compiler *compiler)
  {
    for (auto const &compiler_setup_function : compiler_setup_functions_)
    {
      compiler_setup_function(compiler);
    }
  }

  void VMSetup(VM *vm)
  {
    for (auto const &info : opcode_handler_info_array_)
    {
      vm->AddOpcodeHandler(info);
    }
    vm->SetRegisteredTypes(registered_types_);
  }

  using CompilerSetupFunction = std::function<void(Compiler *)>;

  TypeId GetNextTypeId()
  {
    return next_type_id_++;
  }

  Opcode GetNextOpcode()
  {
    return next_opcode_++;
  }

  void RegisterType(TypeIndex type_index, TypeId type_id)
  {
    registered_types_.RegisterType(type_index, type_id);
  }

  TypeId GetTypeId(TypeIndex type_index) const
  {
    TypeId const type_id = registered_types_.GetTypeId(type_index);
    if (type_id != TypeIds::Unknown)
    {
      return type_id;
    }
    throw std::runtime_error("type index has not been registered");
  }

  TypeIdArray GetTypeIds(TypeIndexArray const &type_index_array) const
  {
    TypeIdArray ids;
    for (auto type_index : type_index_array)
    {
      ids.push_back(GetTypeId(type_index));
    }
    return ids;
  }

  void AddCompilerSetupFunction(CompilerSetupFunction function)
  {
    compiler_setup_functions_.push_back(function);
  }

  void AddOpcodeHandler(Opcode opcode, OpcodeHandler handler)
  {
    opcode_handler_info_array_.push_back(OpcodeHandlerInfo(opcode, handler));
  }

  TypeId                             next_type_id_;
  Opcode                             next_opcode_;
  RegisteredTypes                    registered_types_;
  std::vector<CompilerSetupFunction> compiler_setup_functions_;
  std::vector<OpcodeHandlerInfo>     opcode_handler_info_array_;

  friend class Compiler;
  friend class VM;
};

}  // namespace vm
}  // namespace fetch
