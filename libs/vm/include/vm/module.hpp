#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "analyser.hpp"
#include "defs.hpp"
#include "vm.hpp"

// Class export
#include "module/class_constructor_export.hpp"
#include "module/class_member_export.hpp"
#include "module/wrapper_class.hpp"

// Functions and static functions
#include "module/function_export.hpp"

// VM and analyser related
#include "module/argument_list.hpp"
#include "module/stack_loader.hpp"

#include <functional>
#include <iostream>

namespace fetch {
namespace vm {

class BaseClassInterface : public std::enable_shared_from_this<BaseClassInterface>
{
public:
  BaseClassInterface() = default;
};

template <typename T>
class ClassInterface;
class Analyser;
class VM;

class Module
{
public:
  Module(Opcode const &first_opcode  = Opcode::StartOfUserOpcodes,
         TypeId        first_type_id = TypeId::StartOfUserTypes)
    : first_opcode_(first_opcode)
    , current_opcode_(first_opcode)
    , current_type_id_(std::move(first_type_id))
  {
    opcode_functions_.reserve(1024);
  }

  template <typename T>
  ClassInterface<T> &ExportClass(std::string name);

  template <typename F>
  Module &ExportFunction(std::string name, F obj)
  {
    return ExportFunction(name, std::move(obj), &F::operator());
  }

  template <typename C, typename R, typename... Args>
  Module &ExportFunction(std::string name, C function, R (C::*pointer)(Args...) const)
  {
    return ExportFunctioImplementation<R, Args...>(name, function);
  }

  template <typename R, typename... Args>
  Module &ExportFunction(std::string name, R (*function)(Args...))
  {
    return ExportFunctioImplementation<R, Args...>(name, *function);
  }

protected:
  template <typename R, typename... Args, typename F>
  Module &ExportFunctioImplementation(std::string name, F function)
  {
    Opcode opcode = next_opcode();

    std::vector<std::type_index> prototype;
    details::ArgumentsToList<Args...>::AppendTo(prototype);

    // Creating logic to map opcodes
    AddSetupFunction([opcode, name, prototype](Analyser *a) {
      std::vector<TypePtr> args;
      for (auto &p : prototype)
      {
        args.push_back(a->GetType(p));
      }
      TypePtr ret_type = a->GetType<R>();

      a->CreateOpcodeFunction(name, args, ret_type, opcode);
    });

    // Logic for executing opcodes
    using function_pointer = F;
    AddOpcode(opcode, [name, function](VM *vm) {
      function_pointer fnc = function;  // Copy to drop const qualifier
      using return_type    = R;

      constexpr int result_position        = int(sizeof...(Args)) - details::HasResult<R>::value;
      constexpr int last_argument_position = int(sizeof...(Args)) - 1;

      // resetter that resets all elements until (but not including) the result position
      using stack_resetter   = typename details::Resetter<result_position>;
      using function_invoker = typename details::StaticOrFreeFunctionMagic<
          function_pointer, return_type, result_position>::template LoopOver<last_argument_position,
                                                                             Args...>;

      function_invoker::Apply(vm, fnc);
      stack_resetter::Reset(vm);

      vm->sp_ -= result_position;
    });

    return *this;
  }

  template <typename T>
  friend class ClassInterface;
  friend class Analyser;
  friend class VM;

  Opcode next_opcode()
  {
    Opcode ret      = current_opcode_;
    current_opcode_ = Opcode(1 + uint64_t(current_opcode_));

    return ret;
  }

  TypeId next_type_id()
  {
    TypeId ret       = current_type_id_;
    current_type_id_ = TypeId(1 + uint64_t(current_type_id_));

    return ret;
  }

  void AddSetupFunction(std::function<void(Analyser *)> fnc) { setup_.push_back(fnc); }

  void Setup(Analyser *a)
  {
    for (auto &f : setup_)
    {
      if (!f)
      {
        throw std::runtime_error("Could not execute function");
      }
      f(a);
    }
  }

  void AddOpcode(Opcode const &opcode, std::function<void(VM *)> fnc)
  {
    std::size_t pos = std::size_t(opcode) - std::size_t(first_opcode_);

    if (pos >= opcode_functions_.size())
    {
      opcode_functions_.resize(pos + 1);
    }
    if (!fnc)
    {
      throw std::runtime_error("function pointer is not valid.");
    }
    opcode_functions_[pos] = fnc;
  }

  bool ExecuteUserOpcode(VM *vm, Opcode const &opcode)
  {
    std::size_t pos = std::size_t(opcode) - std::size_t(first_opcode_);
    if (pos >= opcode_functions_.size())
    {
      return false;
    }

    if (!opcode_functions_[pos])
    {
      return false;
    }

    opcode_functions_[pos](vm);

    return true;
  }

private:
  Opcode first_opcode_;
  Opcode current_opcode_;
  TypeId current_type_id_;

  std::vector<std::function<void(Analyser *)>> setup_;
  std::vector<std::function<void(VM *)>>       opcode_functions_;
};

template <typename T>
class ClassInterface : public BaseClassInterface
{
public:
  ClassInterface(std::string name, Module &module) : module_(module), type_(module.next_type_id())
  {}

  template <typename R, typename... Args>
  ClassInterface &Export(std::string name, R (T::*function)(Args...))
  {
    auto   self   = this->shared_from_this();
    Opcode opcode = module_.next_opcode();

    std::vector<std::type_index> prototype;
    details::ArgumentsToList<Args...>::AppendTo(prototype);

    // Creating logic to map opcodes
    module_.AddSetupFunction([this, self, opcode, name, prototype](Analyser *a) {
      std::vector<TypePtr> args;
      for (auto &p : prototype)
      {
        args.push_back(a->GetType(p));
      }
      TypePtr ret_type = a->GetType<R>();

      a->CreateOpcodeInstanceFunction(type_pointer_, name, args, ret_type, opcode);
    });

    // Logic for executing opcodes
    using member_function_pointer = R (T::*)(Args...);
    module_.AddOpcode(opcode, [name, function](VM *vm) {
      member_function_pointer fnc = function;  // Copy to drop const qualifier

      using return_type = R;
      using class_type  = T;

      constexpr int result_position =
          int(sizeof...(Args)) + 1 - details::HasResult<R>::value;  // +1 is to override this
      constexpr int last_argument_position = int(sizeof...(Args)) - 1;
      constexpr int this_position          = int(sizeof...(Args));

      // resetter that resets all elements until (but not including) the result position
      using stack_resetter   = typename details::Resetter<result_position>;
      using function_invoker = typename details::MemberFunctionMagic<
          class_type, member_function_pointer, return_type,
          result_position>::template LoopOver<last_argument_position, Args...>;

      assert((vm->sp_ - result_position) >= 0);
      assert((vm->sp_ - this_position) >= 0);

      Value &this_element  = vm->stack_[vm->sp_ - this_position];
      auto   class_wrapper = static_cast<WrapperClass<class_type> *>(this_element.variant.object);
      class_type &class_from_stack = class_wrapper->object;

      function_invoker::Apply(vm, class_from_stack, fnc);
      stack_resetter::Reset(vm);

      vm->sp_ -= result_position;
    });

    return *this;
  }

  /*
    template <typename R, typename... Args>
    ClassInterface& ExportStaticFunction(std::string name, R (*function)(Args...) )
    {
      return ExportStaticFunctionImplemenetation<R, Args...>(name, *function);
    }
  */
  template <typename F>
  ClassInterface &ExportStaticFunction(std::string name, F obj)
  {
    return ExportStaticFunction(name, std::move(obj), &F::operator());
  }

  template <typename C, typename R, typename... Args>
  ClassInterface &ExportStaticFunction(std::string name, C function, R (C::*pointer)(Args...) const)
  {
    return ExportStaticFunctionImplementation<R, Args...>(name, function);
  }

  template <typename R, typename... Args>
  ClassInterface &ExportStaticFunction(std::string name, R (*function)(Args...))
  {
    return ExportStaticFunctionImplementation<R, Args...>(name, *function);
  }

  template <typename... Args>
  ClassInterface &Constructor()
  {
    auto self = this->shared_from_this();

    /* Creating a list with the function arguments */
    std::vector<std::type_index> prototype;
    details::ArgumentsToList<Args...>::AppendTo(prototype);
    Opcode opcode = module_.next_opcode();

    // Setting the analyser up with the right functionality
    module_.AddSetupFunction([self, this, opcode, prototype](Analyser *a) {
      std::vector<TypePtr> args;
      for (auto &p : prototype)
      {
        args.push_back(a->GetType(p));
      }

      a->CreateOpcodeTypeFunction(type_pointer_, Analyser::CONSTRUCTOR, args, type_pointer_,
                                  opcode);
    });

    // Logic for executing opcodes
    module_.AddOpcode(opcode, [](VM *vm) {
      T TObj =
          details::ConstructorMagic<T>::template LoopOver<int(sizeof...(Args)) - 1, Args...>::Build(
              vm);
      WrapperClass<T> *obj = new WrapperClass<T>(vm->instruction_->type_id, vm, std::move(TObj));

      int    newsp = vm->sp_ - int(sizeof...(Args)) + 1;
      Value &value = vm->stack_[newsp];
      value.Reset();
      value.SetObject(obj, vm->instruction_->type_id);

      details::Resetter<int(sizeof...(Args))>::Reset(vm);
      vm->sp_ = newsp;
    });

    return *this;
  }

  ClassInterface &Constructor()
  {
    auto self = this->shared_from_this();

    throw std::runtime_error("trivial constructor not implemented yet!!");

    Opcode opcode = module_.next_opcode();
    module_.AddSetupFunction([this, self, opcode](Analyser *a) {
      a->CreateOpcodeTypeFunction(type_pointer_, Analyser::CONSTRUCTOR, {}, type_pointer_, opcode);
    });

    // Logic for executing opcodes
    module_.AddOpcode(opcode, [](VM *vm) {
      Value &value = vm->stack_[++vm->sp_];

      WrapperClass<T> *obj = new WrapperClass<T>(vm->instruction_->type_id, vm, T());
      value.Reset();
      value.SetObject(obj, vm->instruction_->type_id);
    });

    return *this;
  }

  void SetTypePointer(TypePtr ptr) { type_pointer_ = ptr; }

  TypeId  type() const { return type_; }
  TypePtr type_pointer() { return type_pointer_; }

private:
  template <typename R, typename... Args, typename F>
  ClassInterface &ExportStaticFunctionImplementation(std::string name, F function)
  {
    Opcode opcode = module_.next_opcode();

    std::vector<std::type_index> prototype;
    details::ArgumentsToList<Args...>::AppendTo(prototype);

    // Creating logic to map opcodes
    module_.AddSetupFunction([this, opcode, name, prototype](Analyser *a) {
      std::vector<TypePtr> args;
      for (auto &p : prototype)
      {
        args.push_back(a->GetType(p));
      }
      TypePtr ret_type = a->GetType<R>();

      a->CreateOpcodeTypeFunction(type_pointer_, name, args, ret_type, opcode);
    });

    // Logic for executing opcodes
    using function_pointer = F;
    module_.AddOpcode(opcode, [name, function](VM *vm) {
      function_pointer fnc = function;  // Copy to drop const qualifier
      using return_type    = R;

      constexpr int result_position        = int(sizeof...(Args)) - details::HasResult<R>::value;
      constexpr int last_argument_position = int(sizeof...(Args)) - 1;

      // resetter that resets all elements until (but not including) the result position
      using stack_resetter   = typename details::Resetter<result_position>;
      using function_invoker = typename details::StaticOrFreeFunctionMagic<
          function_pointer, return_type, result_position>::template LoopOver<last_argument_position,
                                                                             Args...>;

      assert((vm->sp_ - result_position) >= 0);

      function_invoker::Apply(vm, fnc);
      stack_resetter::Reset(vm);

      vm->sp_ -= result_position;
    });

    return *this;
  }

  Module &module_;
  TypeId  type_;
  TypePtr type_pointer_ = nullptr;
};

template <typename T>
ClassInterface<T> &Module::ExportClass(std::string name)
{
  auto ret = std::make_shared<ClassInterface<T>>(name, *this);

  this->AddSetupFunction([ret, name](Analyser *a) {
    TypePtr ptr = a->CreateClassType(name, ret->type());

    ret->SetTypePointer(ptr);
    a->RegisterType<T>(ptr);
  });

  return *ret;
}

}  // namespace vm
}  // namespace fetch
