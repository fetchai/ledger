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

#include "math/arithmetic/comparison.hpp"
#include "vm/defs.hpp"
#include "vm/io_observer_interface.hpp"
#include "vm/string.hpp"

#include <sstream>

namespace fetch {
namespace vm {

template <typename T, typename = void>
struct Getter;
template <typename T>
struct Getter<T, IfIsPrimitive<T>>
{
  static TypeId GetTypeId(RegisteredTypes const &types, T const & /* parameter */)
  {
    return types.GetTypeId(TypeIndex(typeid(T)));
  }
};
template <typename T>
struct Getter<T, IfIsPtr<T>>
{
  static TypeId GetTypeId(RegisteredTypes const &types, T const & /* parameter */)
  {
    using ManagedType = typename GetManagedType<std::decay_t<T>>::type;
    return types.GetTypeId(TypeIndex(typeid(ManagedType)));
  }
};
template <typename T>
struct Getter<T, typename std::enable_if_t<IsVariant<T>::value>>
{
  static TypeId GetTypeId(RegisteredTypes const & /* types */, T const &parameter)
  {
    return parameter.type_id;
  }
};

template <int POSITION, typename... Ts>
struct AssignParameters;
template <int POSITION, typename T, typename... Ts>
struct AssignParameters<POSITION, T, Ts...>
{
  // Invoked on non-final parameter
  static void Assign(Variant *stack, RegisteredTypes const &types, T const &parameter,
                     Ts const &... parameters)
  {
    TypeId type_id = Getter<T>::GetTypeId(types, parameter);
    if (type_id != TypeIds::Unknown)
    {
      Variant &v = stack[POSITION];
      v.Assign(parameter, type_id);
      AssignParameters<POSITION + 1, Ts...>::Assign(stack, types, parameters...);
    }
  }
};
template <int POSITION, typename T>
struct AssignParameters<POSITION, T>
{
  // Invoked on final parameter
  static void Assign(Variant *stack, RegisteredTypes const &types, T const &parameter)
  {
    TypeId type_id = Getter<T>::GetTypeId(types, parameter);
    if (type_id != TypeIds::Unknown)
    {
      Variant &v = stack[POSITION];
      v.Assign(parameter, type_id);
    }
  }
};
template <int POSITION>
struct AssignParameters<POSITION>
{
  // Invoked on zero parameters
  static void Assign(Variant * /* stack */, RegisteredTypes const & /* types */)
  {}
};

// Forward declarations
class Module;

class ParameterPack
{
public:
  // Construction / Destruction
  explicit ParameterPack(RegisteredTypes const &registered_types)
    : registered_types_{registered_types}
  {}

  ParameterPack(ParameterPack const &) = delete;
  ParameterPack(ParameterPack &&)      = delete;
  ~ParameterPack()                     = default;

  Variant const &operator[](std::size_t index) const
  {
#ifndef NDEBUG
    return params_.at(index);
#else
    return params_[index];
#endif
  }

  std::size_t size() const
  {
    return params_.size();
  }

  template <typename T, typename... Args>
  bool Add(T &&parameter, Args &&... args)
  {
    bool success{false};

    success &= Add(std::forward<T>(parameter));
    success &= Add(std::forward<Args>(args)...);

    return success;
  }

  template <typename T>
  IfIsPrimitive<T, bool> Add(T &&parameter)
  {
    return AddInternal(std::forward<T>(parameter));
  }

  template <typename T>
  IfIsPtr<T, bool> Add(T &&obj)
  {
    bool success{false};

    if (obj)
    {
      success = AddInternal(std::forward<T>(obj));
    }

    return success;
  }

  bool Add()
  {
    return true;
  }

  // Operators
  ParameterPack &operator=(ParameterPack const &) = delete;
  ParameterPack &operator=(ParameterPack &&) = delete;

private:
  template <typename T>
  bool AddInternal(T &&value)
  {
    bool success{false};

    TypeId const type_id = Getter<T>::GetTypeId(registered_types_, value);

    if (TypeIds::Unknown != type_id)
    {
      // add the value to the map
      params_.emplace_back(std::forward<T>(value), type_id);

      // signal great success
      success = true;
    }

    return success;
  }

  using VariantArray = std::vector<Variant>;

  RegisteredTypes const &registered_types_;
  VariantArray           params_{};
};

class VM
{
public:
  VM(Module *module);
  ~VM() = default;

  RegisteredTypes const &registered_types() const
  {
    return registered_types_;
  }

  template <typename... Ts>
  bool Execute(Script const &script, std::string const &name, std::string &error,
               std::string &console_output, Variant &output, Ts const &... parameters)

  {
    ParameterPack parameter_pack{registered_types_};

    if (!parameter_pack.Add(parameters...))
    {
      error = "Unable to generate parameter pack";
      return false;
    }

    return Execute(script, name, error, console_output, output, parameter_pack);
  }

  bool Execute(Script const &script, std::string const &name, std::string &error,
               std::string &console_output, Variant &output, ParameterPack const &parameters)
  {
    bool success{false};

    Script::Function const *f = script.FindFunction(name);
    if (f)
    {
      auto const num_parameters = static_cast<std::size_t>(f->num_parameters);

      if (parameters.size() == num_parameters)
      {
        // loop through the parameters, type check and populate the stack
        for (std::size_t i = 0; i < num_parameters; ++i)
        {
          Variant const &parameter = parameters[i];

          // type check
          if (parameter.type_id != f->variables[i].type_id)
          {
            error = "mismatched parameters";

            // clean up
            for (std::size_t j = 0; j < num_parameters; ++j)
            {
              stack_[j].Reset();
            }

            return false;
          }

          // assign
          stack_[i].Assign(parameter, parameter.type_id);
        }

        script_   = &script;
        function_ = f;

        // execute the function
        success = Execute(error, output);
      }
      else
      {
        error = "mismatched parameters";
      }
    }
    else
    {
      error = "unable to find function '" + name + "'";
    }

    // transfer the console output buffer
    console_output = output_buffer_.str();

    return success;
  }

  template <typename T>
  TypeId GetTypeId()
  {
    return registered_types_.GetTypeId(std::type_index(typeid(T)));
  }

  template <typename T, typename... Args>
  Ptr<T> CreateNewObject(Args &&... args)
  {
    return new T(this, GetTypeId<T>(), std::forward<Args>(args)...);
  }

  void SetIOObserver(IoObserverInterface &observer)
  {
    io_observer_ = &observer;
  }

  bool HasIoObserver() const
  {
    return io_observer_ != nullptr;
  }

  IoObserverInterface &GetIOObserver()
  {
    assert(io_observer_ != nullptr);
    return *io_observer_;
  }

  void AddOutputLine(std::string const &line)
  {
    output_buffer_ << line << '\n';
  }

private:
  static const int FRAME_STACK_SIZE = 50;
  static const int STACK_SIZE       = 5000;
  static const int MAX_LIVE_OBJECTS = 200;
  static const int MAX_RANGE_LOOPS  = 50;

  struct Frame
  {
    Script::Function const *function;
    int                     bsp;
    int                     pc;
  };

  struct ForRangeLoop
  {
    Index     variable_index;
    Primitive current;
    Primitive target;
    Primitive delta;
  };

  struct LiveObjectInfo
  {
    int   frame_sp;
    Index variable_index;
    int   scope_number;
  };

  std::vector<OpcodeHandler> opcode_handlers_;
  RegisteredTypes            registered_types_;
  Script::Function const *   function_;
  std::vector<Ptr<String>>   strings_;
  Frame                      frame_stack_[FRAME_STACK_SIZE];
  int                        frame_sp_;
  int                        bsp_;

  template <typename T>
  friend struct StackGetter;
  template <typename T>
  friend struct StackSetter;
  template <typename T, typename S>
  friend struct TypeGetter;
  template <typename T, typename S>
  friend struct ParameterTypeGetter;
  template <typename ReturnType, typename FreeFunction, typename... Ts>
  friend struct FreeFunctionInvokerHelper;
  template <typename ObjectType, typename ReturnType, typename InstanceFunction, typename... Ts>
  friend struct InstanceFunctionInvokerHelper;
  template <typename ObjectType, typename ReturnType, typename TypeConstructor, typename... Ts>
  friend struct TypeConstructorInvokerHelper;
  template <typename ReturnType, typename TypeFunction, typename... Ts>
  friend struct TypeFunctionInvokerHelper;

  Script const *script_;
  Variant       stack_[STACK_SIZE];
  int           sp_;

  ForRangeLoop               range_loop_stack_[MAX_RANGE_LOOPS];
  int                        range_loop_sp_;
  LiveObjectInfo             live_object_stack_[MAX_LIVE_OBJECTS];
  int                        live_object_sp_;
  int                        pc_;
  Script::Instruction const *instruction_;
  bool                       stop_;
  std::string                error_;
  std::ostringstream         output_buffer_;

  IoObserverInterface *io_observer_{nullptr};

  bool Execute(std::string &error, Variant &output);
  void Destruct(int scope_number);

  Variant &Push()
  {
    return stack_[++sp_];
  }

  Variant &Pop()
  {
    return stack_[sp_--];
  }

  Variant &Top()
  {
    return stack_[sp_];
  }

  // fix these -- should be private
public:
  void            RuntimeError(std::string const &message);
  TypeInfo const &GetTypeInfo(TypeId type_id)
  {
    auto it = script_->type_info_table.find(type_id);
    return it->second;
  }

private:
  // fix these

  void AddOpcodeHandler(OpcodeHandlerInfo const &info)
  {
    if (info.opcode >= opcode_handlers_.size())
    {
      opcode_handlers_.resize(size_t(info.opcode + 1));
    }
    opcode_handlers_[info.opcode] = info.handler;
  }

  void SetRegisteredTypes(RegisteredTypes const &registered_types)
  {
    registered_types_ = registered_types;
  }

  Variant &GetVariable(Index variable_index)
  {
    return stack_[bsp_ + variable_index];
  }

  template <typename From, typename To>
  void PerformCast(From const &from, To &to)
  {
    to = static_cast<To>(from);
  }

  template <typename To>
  void Cast(Variant &v, TypeId to_type_id, To &to)
  {
    TypeId from_type_id = v.type_id;
    v.type_id           = to_type_id;
    switch (from_type_id)
    {
    case TypeIds::Bool:
    {
      PerformCast(v.primitive.ui8, to);
      break;
    }
    case TypeIds::Int8:
    {
      PerformCast(v.primitive.i8, to);
      break;
    }
    case TypeIds::Byte:
    {
      PerformCast(v.primitive.ui8, to);
      break;
    }
    case TypeIds::Int16:
    {
      PerformCast(v.primitive.i16, to);
      break;
    }
    case TypeIds::UInt16:
    {
      PerformCast(v.primitive.ui16, to);
      break;
    }
    case TypeIds::Int32:
    {
      PerformCast(v.primitive.i32, to);
      break;
    }
    case TypeIds::UInt32:
    {
      PerformCast(v.primitive.ui32, to);
      break;
    }
    case TypeIds::Int64:
    {
      PerformCast(v.primitive.i64, to);
      break;
    }
    case TypeIds::UInt64:
    {
      PerformCast(v.primitive.ui64, to);
      break;
    }
    case TypeIds::Float32:
    {
      PerformCast(v.primitive.f32, to);
      break;
    }
    case TypeIds::Float64:
    {
      PerformCast(v.primitive.f64, to);
      break;
    }
    default:
    {
      break;
    }
    }  // switch
  }

  struct EqualOp
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsEqual(lhs, rhs), TypeIds::Bool);
    }
  };

  struct NotEqualOp
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsNotEqual(lhs, rhs), TypeIds::Bool);
    }
  };

  bool IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) const
  {
    if (lhso)
    {
      if (rhso)
      {
        return lhso->IsEqual(lhso, rhso);
      }
      return false;
    }
    return (rhso == nullptr);
  }

  bool IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) const
  {
    if (lhso)
    {
      if (rhso)
      {
        return lhso->IsNotEqual(lhso, rhso);
      }
      return true;
    }
    return (rhso != nullptr);
  }

  struct LessThanOp
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsLessThan(lhs, rhs), TypeIds::Bool);
    }
  };

  struct ObjectLessThanOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.Assign(lhsv.object->IsLessThan(lhsv.object, rhsv.object), TypeIds::Bool);
    }
  };

  struct LessThanOrEqualOp
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsLessThanOrEqual(lhs, rhs), TypeIds::Bool);
    }
  };

  struct ObjectLessThanOrEqualOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.Assign(lhsv.object->IsLessThanOrEqual(lhsv.object, rhsv.object), TypeIds::Bool);
    }
  };

  struct GreaterThanOp
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsGreaterThan(lhs, rhs), TypeIds::Bool);
    }
  };

  struct ObjectGreaterThanOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.Assign(lhsv.object->IsGreaterThan(lhsv.object, rhsv.object), TypeIds::Bool);
    }
  };

  struct GreaterThanOrEqualOp
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsGreaterThanOrEqual(lhs, rhs), TypeIds::Bool);
    }
  };

  struct ObjectGreaterThanOrEqualOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.Assign(lhsv.object->IsGreaterThanOrEqual(lhsv.object, rhsv.object), TypeIds::Bool);
    }
  };

  struct PrefixIncOp
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      rhs = ++lhs;
    }
  };

  struct PrefixDecOp
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      rhs = --lhs;
    }
  };

  struct PostfixIncOp
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      rhs = lhs++;
    }
  };

  struct PostfixDecOp
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      rhs = lhs--;
    }
  };

  struct ModuloOp
  {
    template <typename T>
    static void Apply(VM *vm, T &lhs, T &rhs)
    {
      if (rhs != 0)
      {
        lhs = T(lhs % rhs);
        return;
      }
      vm->RuntimeError("division by zero");
    }
  };

  struct UnaryMinusOp
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T & /* rhs */)
    {
      lhs = T(-lhs);
    }
  };

  struct AddOp
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      lhs = T(lhs + rhs);
    }
  };

  struct ObjectAddOp
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->Add(lhso, rhso);
    }
  };

  struct LeftAddOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftAdd(lhsv, rhsv);
    }
  };

  struct RightAddOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightAdd(lhsv, rhsv);
    }
  };

  struct ObjectAddAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->AddAssign(lhso, rhso);
    }
  };

  struct RightAddAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->RightAddAssign(lhso, rhsv);
    }
  };

  struct SubtractOp
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      lhs = T(lhs - rhs);
    }
  };

  struct ObjectSubtractOp
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->Subtract(lhso, rhso);
    }
  };

  struct LeftSubtractOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftSubtract(lhsv, rhsv);
    }
  };

  struct RightSubtractOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightSubtract(lhsv, rhsv);
    }
  };

  struct ObjectSubtractAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->SubtractAssign(lhso, rhso);
    }
  };

  struct RightSubtractAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->RightSubtractAssign(lhso, rhsv);
    }
  };

  struct MultiplyOp
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      lhs = T(lhs * rhs);
    }
  };

  struct ObjectMultiplyOp
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->Multiply(lhso, rhso);
    }
  };

  struct LeftMultiplyOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftMultiply(lhsv, rhsv);
    }
  };

  struct RightMultiplyOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightMultiply(lhsv, rhsv);
    }
  };

  struct ObjectMultiplyAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->MultiplyAssign(lhso, rhso);
    }
  };

  struct RightMultiplyAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->RightMultiplyAssign(lhso, rhsv);
    }
  };

  struct DivideOp
  {
    template <typename T>
    static void Apply(VM *vm, T &lhs, T &rhs)
    {
      if (math::IsNonZero(rhs))
      {
        lhs = T(lhs / rhs);
        return;
      }
      vm->RuntimeError("division by zero");
    }
  };

  struct ObjectDivideOp
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->Divide(lhso, rhso);
    }
  };

  struct LeftDivideOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftDivide(lhsv, rhsv);
    }
  };

  struct RightDivideOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightDivide(lhsv, rhsv);
    }
  };

  struct ObjectDivideAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->DivideAssign(lhso, rhso);
    }
  };

  struct RightDivideAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->RightDivideAssign(lhso, rhsv);
    }
  };

  template <typename Op>
  void ExecuteRelationalOp(TypeId type_id, Variant &lhsv, Variant &rhsv)
  {
    switch (type_id)
    {
    case TypeIds::Bool:
    {
      Op::Apply(lhsv, lhsv.primitive.ui8, rhsv.primitive.ui8);
      break;
    }
    case TypeIds::Int8:
    {
      Op::Apply(lhsv, lhsv.primitive.i8, rhsv.primitive.i8);
      break;
    }
    case TypeIds::Byte:
    {
      Op::Apply(lhsv, lhsv.primitive.ui8, rhsv.primitive.ui8);
      break;
    }
    case TypeIds::Int16:
    {
      Op::Apply(lhsv, lhsv.primitive.i16, rhsv.primitive.i16);
      break;
    }
    case TypeIds::UInt16:
    {
      Op::Apply(lhsv, lhsv.primitive.ui16, rhsv.primitive.ui16);
      break;
    }
    case TypeIds::Int32:
    {
      Op::Apply(lhsv, lhsv.primitive.i32, rhsv.primitive.i32);
      break;
    }
    case TypeIds::UInt32:
    {
      Op::Apply(lhsv, lhsv.primitive.ui32, rhsv.primitive.ui32);
      break;
    }
    case TypeIds::Int64:
    {
      Op::Apply(lhsv, lhsv.primitive.i64, rhsv.primitive.i64);
      break;
    }
    case TypeIds::UInt64:
    {
      Op::Apply(lhsv, lhsv.primitive.ui64, rhsv.primitive.ui64);
      break;
    }
    case TypeIds::Float32:
    {
      Op::Apply(lhsv, lhsv.primitive.f32, rhsv.primitive.f32);
      break;
    }
    case TypeIds::Float64:
    {
      Op::Apply(lhsv, lhsv.primitive.f64, rhsv.primitive.f64);
      break;
    }
    default:
    {
      break;
    }
    }  // switch
  }

  template <typename Op>
  void ExecuteIntegerOp(TypeId type_id, Variant &lhsv, Variant &rhsv)
  {
    switch (type_id)
    {
    case TypeIds::Int8:
    {
      Op::Apply(this, lhsv.primitive.i8, rhsv.primitive.i8);
      break;
    }
    case TypeIds::Byte:
    {
      Op::Apply(this, lhsv.primitive.ui8, rhsv.primitive.ui8);
      break;
    }
    case TypeIds::Int16:
    {
      Op::Apply(this, lhsv.primitive.i16, rhsv.primitive.i16);
      break;
    }
    case TypeIds::UInt16:
    {
      Op::Apply(this, lhsv.primitive.ui16, rhsv.primitive.ui16);
      break;
    }
    case TypeIds::Int32:
    {
      Op::Apply(this, lhsv.primitive.i32, rhsv.primitive.i32);
      break;
    }
    case TypeIds::UInt32:
    {
      Op::Apply(this, lhsv.primitive.ui32, rhsv.primitive.ui32);
      break;
    }
    case TypeIds::Int64:
    {
      Op::Apply(this, lhsv.primitive.i64, rhsv.primitive.i64);
      break;
    }
    case TypeIds::UInt64:
    {
      Op::Apply(this, lhsv.primitive.ui64, rhsv.primitive.ui64);
      break;
    }
    default:
    {
      break;
    }
    }  // switch
  }

  template <typename Op>
  void ExecuteNumberOp(TypeId type_id, Variant &lhsv, Variant &rhsv)
  {
    switch (type_id)
    {
    case TypeIds::Int8:
    {
      Op::Apply(this, lhsv.primitive.i8, rhsv.primitive.i8);
      break;
    }
    case TypeIds::Byte:
    {
      Op::Apply(this, lhsv.primitive.ui8, rhsv.primitive.ui8);
      break;
    }
    case TypeIds::Int16:
    {
      Op::Apply(this, lhsv.primitive.i16, rhsv.primitive.i16);
      break;
    }
    case TypeIds::UInt16:
    {
      Op::Apply(this, lhsv.primitive.ui16, rhsv.primitive.ui16);
      break;
    }
    case TypeIds::Int32:
    {
      Op::Apply(this, lhsv.primitive.i32, rhsv.primitive.i32);
      break;
    }
    case TypeIds::UInt32:
    {
      Op::Apply(this, lhsv.primitive.ui32, rhsv.primitive.ui32);
      break;
    }
    case TypeIds::Int64:
    {
      Op::Apply(this, lhsv.primitive.i64, rhsv.primitive.i64);
      break;
    }
    case TypeIds::UInt64:
    {
      Op::Apply(this, lhsv.primitive.ui64, rhsv.primitive.ui64);
      break;
    }
    case TypeIds::Float32:
    {
      Op::Apply(this, lhsv.primitive.f32, rhsv.primitive.f32);
      break;
    }
    case TypeIds::Float64:
    {
      Op::Apply(this, lhsv.primitive.f64, rhsv.primitive.f64);
      break;
    }
    default:
    {
      break;
    }
    }  // switch
  }

  template <typename Op>
  void ExecuteIntegerAssignOp(TypeId type_id, void *lhs, Variant &rhsv)
  {
    switch (type_id)
    {
    case TypeIds::Int8:
    {
      Op::Apply(this, *static_cast<int8_t *>(lhs), rhsv.primitive.i8);
      break;
    }
    case TypeIds::Byte:
    {
      Op::Apply(this, *static_cast<uint8_t *>(lhs), rhsv.primitive.ui8);
      break;
    }
    case TypeIds::Int16:
    {
      Op::Apply(this, *static_cast<int16_t *>(lhs), rhsv.primitive.i16);
      break;
    }
    case TypeIds::UInt16:
    {
      Op::Apply(this, *static_cast<uint16_t *>(lhs), rhsv.primitive.ui16);
      break;
    }
    case TypeIds::Int32:
    {
      Op::Apply(this, *static_cast<int32_t *>(lhs), rhsv.primitive.i32);
      break;
    }
    case TypeIds::UInt32:
    {
      Op::Apply(this, *static_cast<uint32_t *>(lhs), rhsv.primitive.ui32);
      break;
    }
    case TypeIds::Int64:
    {
      Op::Apply(this, *static_cast<int64_t *>(lhs), rhsv.primitive.i64);
      break;
    }
    case TypeIds::UInt64:
    {
      Op::Apply(this, *static_cast<uint64_t *>(lhs), rhsv.primitive.ui64);
      break;
    }
    default:
    {
      break;
    }
    }  // switch
  }

  template <typename Op>
  void ExecuteNumberAssignOp(TypeId type_id, void *lhs, Variant &rhsv)
  {
    switch (type_id)
    {
    case TypeIds::Int8:
    {
      Op::Apply(this, *static_cast<int8_t *>(lhs), rhsv.primitive.i8);
      break;
    }
    case TypeIds::Byte:
    {
      Op::Apply(this, *static_cast<uint8_t *>(lhs), rhsv.primitive.ui8);
      break;
    }
    case TypeIds::Int16:
    {
      Op::Apply(this, *static_cast<int16_t *>(lhs), rhsv.primitive.i16);
      break;
    }
    case TypeIds::UInt16:
    {
      Op::Apply(this, *static_cast<uint16_t *>(lhs), rhsv.primitive.ui16);
      break;
    }
    case TypeIds::Int32:
    {
      Op::Apply(this, *static_cast<int32_t *>(lhs), rhsv.primitive.i32);
      break;
    }
    case TypeIds::UInt32:
    {
      Op::Apply(this, *static_cast<uint32_t *>(lhs), rhsv.primitive.ui32);
      break;
    }
    case TypeIds::Int64:
    {
      Op::Apply(this, *static_cast<int64_t *>(lhs), rhsv.primitive.i64);
      break;
    }
    case TypeIds::UInt64:
    {
      Op::Apply(this, *static_cast<uint64_t *>(lhs), rhsv.primitive.ui64);
      break;
    }
    case TypeIds::Float32:
    {
      Op::Apply(this, *static_cast<float *>(lhs), rhsv.primitive.f32);
      break;
    }
    case TypeIds::Float64:
    {
      Op::Apply(this, *static_cast<double *>(lhs), rhsv.primitive.f64);
      break;
    }
    default:
    {
      break;
    }
    }  // switch
  }

  template <typename Op>
  void DoRelationalOp()
  {
    Variant &rhsv = Pop();
    Variant &lhsv = Top();
    ExecuteRelationalOp<Op>(instruction_->type_id, lhsv, rhsv);
    rhsv.Reset();
  }

  template <typename Op>
  void DoObjectRelationalOp()
  {
    Variant &rhsv = Pop();
    Variant &lhsv = Top();
    if (lhsv.object && rhsv.object)
    {
      Op::Apply(lhsv, rhsv);
      rhsv.Reset();
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoIncDecOp(TypeId type_id, void *lhs)
  {
    Variant &rhsv = Push();
    ExecuteIntegerAssignOp<Op>(type_id, lhs, rhsv);
    rhsv.type_id = instruction_->type_id;
  }

  template <typename Op>
  void DoVariableIncDecOp()
  {
    Variant &variable = GetVariable(instruction_->index);
    DoIncDecOp<Op>(instruction_->type_id, &variable.primitive);
  }

  template <typename Op>
  void DoElementIncDecOp()
  {
    Variant &container = Pop();
    if (container.object)
    {
      void *element = container.object->FindElement();
      if (element)
      {
        DoIncDecOp<Op>(instruction_->type_id, element);
        container.Reset();
      }
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoIntegerOp()
  {
    Variant &rhsv = Pop();
    Variant &lhsv = Top();
    ExecuteIntegerOp<Op>(instruction_->type_id, lhsv, rhsv);
    rhsv.Reset();
  }

  template <typename Op>
  void DoNumberOp()
  {
    Variant &rhsv = Pop();
    Variant &lhsv = Top();
    ExecuteNumberOp<Op>(instruction_->type_id, lhsv, rhsv);
    rhsv.Reset();
  }

  template <typename Op>
  void DoLeftOp()
  {
    Variant &rhsv = Pop();
    Variant &lhsv = Top();
    if (rhsv.object)
    {
      Op::Apply(lhsv, rhsv);
      rhsv.Reset();
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoRightOp()
  {
    Variant &rhsv = Pop();
    Variant &lhsv = Top();
    if (lhsv.object)
    {
      Op::Apply(lhsv, rhsv);
      rhsv.Reset();
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoObjectOp()
  {
    Variant &rhsv = Pop();
    Variant &lhsv = Top();
    if (lhsv.object && rhsv.object)
    {
      Op::Apply(lhsv.object, rhsv.object);
      rhsv.Reset();
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoIntegerAssignOp(TypeId type_id, void *lhs)
  {
    Variant &rhsv = Pop();
    ExecuteIntegerAssignOp<Op>(type_id, lhs, rhsv);
    rhsv.Reset();
  }

  template <typename Op>
  void DoNumberAssignOp(TypeId type_id, void *lhs)
  {
    Variant &rhsv = Pop();
    ExecuteNumberAssignOp<Op>(type_id, lhs, rhsv);
    rhsv.Reset();
  }

  template <typename Op>
  void DoRightAssignOp(Ptr<Object> &lhso)
  {
    Variant &rhsv = Pop();
    if (lhso)
    {
      Op::Apply(lhso, rhsv);
      rhsv.Reset();
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoObjectAssignOp(Ptr<Object> &lhso)
  {
    Variant &rhsv = Pop();
    if (lhso && rhsv.object)
    {
      Op::Apply(lhso, rhsv.object);
      rhsv.Reset();
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoVariableIntegerAssignOp()
  {
    Variant &variable = GetVariable(instruction_->index);
    DoIntegerAssignOp<Op>(instruction_->type_id, &variable.primitive);
  }

  template <typename Op>
  void DoVariableNumberAssignOp()
  {
    Variant &variable = GetVariable(instruction_->index);
    DoNumberAssignOp<Op>(instruction_->type_id, &variable.primitive);
  }

  template <typename Op>
  void DoVariableRightAssignOp()
  {
    Variant &variable = GetVariable(instruction_->index);
    DoRightAssignOp<Op>(variable.object);
  }

  template <typename Op>
  void DoVariableObjectAssignOp()
  {
    Variant &variable = GetVariable(instruction_->index);
    DoObjectAssignOp<Op>(variable.object);
  }

  template <typename Op>
  void DoElementIntegerAssignOp()
  {
    Variant &container = Pop();
    if (container.object)
    {
      void *element = container.object->FindElement();
      if (element)
      {
        DoIntegerAssignOp<Op>(instruction_->type_id, element);
        container.Reset();
      }
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoElementNumberAssignOp()
  {
    Variant &container = Pop();
    if (container.object)
    {
      void *element = container.object->FindElement();
      if (element)
      {
        DoNumberAssignOp<Op>(instruction_->type_id, element);
        container.Reset();
      }
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoElementRightAssignOp()
  {
    Variant &container = Pop();
    if (container.object)
    {
      Ptr<Object> *element = static_cast<Ptr<Object> *>(container.object->FindElement());
      if (element)
      {
        DoRightAssignOp<Op>(*element);
        container.Reset();
      }
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoElementObjectAssignOp()
  {
    Variant &container = Pop();
    if (container.object)
    {
      Ptr<Object> *element = static_cast<Ptr<Object> *>(container.object->FindElement());
      if (element)
      {
        DoObjectAssignOp<Op>(*element);
        container.Reset();
      }
      return;
    }
    RuntimeError("null reference");
  }

  //
  // Opcode handler prototypes
  //

  void VarDeclare();
  void VarDeclareAssign();
  void PushConstant();
  void PushString();
  void PushNull();
  void PushVariable();
  void PushElement();
  void PopToVariable();
  void PopToElement();
  void Discard();
  void Destruct();
  void Break();
  void Continue();
  void Jump();
  void JumpIfFalse();
  void JumpIfTrue();
  void Return();
  void ToInt8();
  void ToByte();
  void ToInt16();
  void ToUInt16();
  void ToInt32();
  void ToUInt32();
  void ToInt64();
  void ToUInt64();
  void ToFloat32();
  void ToFloat64();
  void ForRangeInit();
  void ForRangeIterate();
  void ForRangeTerminate();
  void InvokeUserFunction();
  void Equal();
  void ObjectEqual();
  void NotEqual();
  void ObjectNotEqual();
  void LessThan();
  void ObjectLessThan();
  void LessThanOrEqual();
  void ObjectLessThanOrEqual();
  void GreaterThan();
  void ObjectGreaterThan();
  void GreaterThanOrEqual();
  void ObjectGreaterThanOrEqual();
  void And();
  void Or();
  void Not();
  void VariablePrefixInc();
  void VariablePrefixDec();
  void VariablePostfixInc();
  void VariablePostfixDec();
  void ElementPrefixInc();
  void ElementPrefixDec();
  void ElementPostfixInc();
  void ElementPostfixDec();
  void Modulo();
  void VariableModuloAssign();
  void ElementModuloAssign();
  void UnaryMinus();
  void ObjectUnaryMinus();
  void Add();
  void LeftAdd();
  void RightAdd();
  void ObjectAdd();
  void VariableAddAssign();
  void VariableRightAddAssign();
  void VariableObjectAddAssign();
  void ElementAddAssign();
  void ElementRightAddAssign();
  void ElementObjectAddAssign();
  void Subtract();
  void LeftSubtract();
  void RightSubtract();
  void ObjectSubtract();
  void VariableSubtractAssign();
  void VariableRightSubtractAssign();
  void VariableObjectSubtractAssign();
  void ElementSubtractAssign();
  void ElementRightSubtractAssign();
  void ElementObjectSubtractAssign();
  void Multiply();
  void LeftMultiply();
  void RightMultiply();
  void ObjectMultiply();
  void VariableMultiplyAssign();
  void VariableRightMultiplyAssign();
  void VariableObjectMultiplyAssign();
  void ElementMultiplyAssign();
  void ElementRightMultiplyAssign();
  void ElementObjectMultiplyAssign();
  void Divide();
  void LeftDivide();
  void RightDivide();
  void ObjectDivide();
  void VariableDivideAssign();
  void VariableRightDivideAssign();
  void VariableObjectDivideAssign();
  void ElementDivideAssign();
  void ElementRightDivideAssign();
  void ElementObjectDivideAssign();

  friend class Object;
  friend class Module;
};

}  // namespace vm
}  // namespace fetch
