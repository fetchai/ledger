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
#include "math/free_functions/free_functions.hpp"
#include "vm/defs.hpp"
#include "vm/state_sentinel.hpp"
#include "vm/string.hpp"

namespace fetch {
namespace vm {

template <typename T, typename = void>
struct Getter;
template <typename T>
struct Getter<T, typename std::enable_if_t<IsPrimitive<std::decay_t<T>>::value>>
{
  static TypeIndex GetTypeIndex()
  {
    return TypeIndex(typeid(std::decay_t<T>));
  }
};
template <typename T>
struct Getter<T, typename std::enable_if_t<IsPtr<std::decay_t<T>>::value>>
{
  static TypeIndex GetTypeIndex()
  {
    using ManagedType = typename GetManagedType<std::decay_t<T>>::type;
    return TypeIndex(typeid(ManagedType));
  }
};

template <int POSITION, typename... Ts>
struct AssignParameters;
template <int POSITION, typename T, typename... Ts>
struct AssignParameters<POSITION, T, Ts...>
{
  // Invoked on non-final parameter
  static void Assign(Variant *stack, RegisteredTypes &types, T const &parameter,
                     Ts const &... parameters)
  {
    TypeIndex type_index = Getter<T>::GetTypeIndex();
    TypeId    type_id    = types.GetTypeId(type_index);
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
  static void Assign(Variant *stack, RegisteredTypes &types, T const &parameter)
  {
    TypeIndex type_index = Getter<T>::GetTypeIndex();
    TypeId    type_id    = types.GetTypeId(type_index);
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
  static void Assign(Variant * /* stack */, RegisteredTypes & /* types */)
  {}
};

// Forward declarations
class Module;

class VM
{
public:
  VM(Module *module);
  ~VM() = default;

  template <typename... Ts>
  bool Execute(Script const &script, std::string const &name, std::string &error,
               std::vector<std::string> &print_strings, Variant &output, Ts const &... parameters)
  {
    print_strings_ = &print_strings;
    print_strings_->clear();

    Script::Function const *f = script.FindFunction(name);
    if (f == nullptr)
    {
      error = "unable to find function '" + name + "'";
      return false;
    }
    constexpr int num_parameters = int(sizeof...(Ts));
    if (num_parameters != f->num_parameters)
    {
      error = "mismatched parameters";
      return false;
    }
    AssignParameters<0, Ts...>::Assign(stack_, registered_types_, parameters...);
    for (size_t i = 0; i < size_t(num_parameters); ++i)
    {
      if (stack_[i].type_id != f->variables[i].type_id)
      {
        error = "mismatched parameters";
        for (size_t j = 0; j < size_t(num_parameters); ++j)
        {
          stack_[j].Reset();
        }
        return false;
      }
    }
    script_   = &script;
    function_ = f;
    return Execute(error, output);
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

  void SetIOInterface(ReadWriteInterface *ptr)
  {
    state_sentinel_.SetReadWriteInterface(ptr);
  }

  ReadWriteInterface *GetIOInterface()
  {
    return state_sentinel_.GetReadWriteInterface();
  }

  void AddPrintString(std::string const &print_string)
  {
    if (print_strings_)
    {
      print_strings_->push_back(print_string);
    }
  }

protected:
  template <typename T>
  friend class State;
  StateSentinel state_sentinel_;

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

private:
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
  std::vector<std::string> * print_strings_ = nullptr;

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

  struct PrimitiveEqualOp
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsEqual(lhs, rhs), TypeIds::Bool);
    }
  };

  struct PrimitiveNotEqualOp
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsNotEqual(lhs, rhs), TypeIds::Bool);
    }
  };

  struct PrimitiveLessThanOp
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsLessThan(lhs, rhs), TypeIds::Bool);
    }
  };

  struct PrimitiveLessThanOrEqualOp
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsLessThanOrEqual(lhs, rhs), TypeIds::Bool);
    }
  };

  struct PrimitiveGreaterThanOp
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsGreaterThan(lhs, rhs), TypeIds::Bool);
    }
  };

  struct PrimitiveGreaterThanOrEqualOp
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsGreaterThanOrEqual(lhs, rhs), TypeIds::Bool);
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

  struct PrimitiveUnaryMinusOp
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T & /* rhs */)
    {
      lhs = T(-lhs);
    }
  };

  struct PrimitiveAddOp
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
      lhso->AddOp(lhso, rhso);
    }
  };

  struct LeftAddOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftAddOp(lhsv, rhsv);
    }
  };

  struct RightAddOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightAddOp(lhsv, rhsv);
    }
  };

  struct ObjectAddAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->AddAssignOp(lhso, rhso);
    }
  };

  struct RightAddAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->RightAddAssignOp(lhso, rhsv);
    }
  };

  struct PrimitiveSubtractOp
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
      lhso->SubtractOp(lhso, rhso);
    }
  };

  struct LeftSubtractOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftSubtractOp(lhsv, rhsv);
    }
  };

  struct RightSubtractOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightSubtractOp(lhsv, rhsv);
    }
  };

  struct ObjectSubtractAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->SubtractAssignOp(lhso, rhso);
    }
  };

  struct RightSubtractAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->RightSubtractAssignOp(lhso, rhsv);
    }
  };

  struct PrimitiveMultiplyOp
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
      lhso->MultiplyOp(lhso, rhso);
    }
  };

  struct LeftMultiplyOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftMultiplyOp(lhsv, rhsv);
    }
  };

  struct RightMultiplyOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightMultiplyOp(lhsv, rhsv);
    }
  };

  struct ObjectMultiplyAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->MultiplyAssignOp(lhso, rhso);
    }
  };

  struct RightMultiplyAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->RightMultiplyAssignOp(lhso, rhsv);
    }
  };

  struct PrimitiveDivideOp
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
      lhso->DivideOp(lhso, rhso);
    }
  };

  struct LeftDivideOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftDivideOp(lhsv, rhsv);
    }
  };

  struct RightDivideOp
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightDivideOp(lhsv, rhsv);
    }
  };

  struct ObjectDivideAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->DivideAssignOp(lhso, rhso);
    }
  };

  struct RightDivideAssignOp
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->RightDivideAssignOp(lhso, rhsv);
    }
  };

  template <typename Op>
  void ExecutePrimitiveLogicalOp(TypeId type_id, Variant &lhsv, Variant &rhsv)
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
  void ExecutePrimitiveOp(TypeId type_id, Variant &lhsv, Variant &rhsv)
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
  void ExecutePrimitiveAssignOp(TypeId type_id, void *lhs, Variant &rhsv)
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
  void DoPrimitiveLogicalOp()
  {
    Variant &rhsv = Pop();
    Variant &lhsv = Top();
    ExecutePrimitiveLogicalOp<Op>(instruction_->type_id, lhsv, rhsv);
    rhsv.Reset();
  }

  template <typename Op>
  void DoIncDecOp(TypeId type_id, void *lhs)
  {
    Variant &rhsv = Push();
    ExecutePrimitiveAssignOp<Op>(type_id, lhs, rhsv);
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
  void DoPrimitiveOp()
  {
    Variant &rhsv = Pop();
    Variant &lhsv = Top();
    ExecutePrimitiveOp<Op>(instruction_->type_id, lhsv, rhsv);
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
  void DoPrimitiveAssignOp(TypeId type_id, void *lhs)
  {
    Variant &rhsv = Pop();
    ExecutePrimitiveAssignOp<Op>(type_id, lhs, rhsv);
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
  void DoVariablePrimitiveAssignOp()
  {
    Variant &variable = GetVariable(instruction_->index);
    DoPrimitiveAssignOp<Op>(instruction_->type_id, &variable.primitive);
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
  void DoElementPrimitiveAssignOp()
  {
    Variant &container = Pop();
    if (container.object)
    {
      void *element = container.object->FindElement();
      if (element)
      {
        DoPrimitiveAssignOp<Op>(instruction_->type_id, element);
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

  bool IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) const
  {
    if (lhso)
    {
      if (rhso)
      {
        return lhso->Equals(lhso, rhso);
      }
      return false;
    }
    return !rhso;
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
  void PrimitiveEqual();
  void ObjectEqual();
  void PrimitiveNotEqual();
  void ObjectNotEqual();
  void PrimitiveLessThan();
  void PrimitiveLessThanOrEqual();
  void PrimitiveGreaterThan();
  void PrimitiveGreaterThanOrEqual();
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
  void PrimitiveUnaryMinus();
  void ObjectUnaryMinus();
  void PrimitiveAdd();
  void LeftAdd();
  void RightAdd();
  void ObjectAdd();
  void VariablePrimitiveAddAssign();
  void VariableRightAddAssign();
  void VariableObjectAddAssign();
  void ElementPrimitiveAddAssign();
  void ElementRightAddAssign();
  void ElementObjectAddAssign();
  void PrimitiveSubtract();
  void LeftSubtract();
  void RightSubtract();
  void ObjectSubtract();
  void VariablePrimitiveSubtractAssign();
  void VariableRightSubtractAssign();
  void VariableObjectSubtractAssign();
  void ElementPrimitiveSubtractAssign();
  void ElementRightSubtractAssign();
  void ElementObjectSubtractAssign();
  void PrimitiveMultiply();
  void LeftMultiply();
  void RightMultiply();
  void ObjectMultiply();
  void VariablePrimitiveMultiplyAssign();
  void VariableRightMultiplyAssign();
  void VariableObjectMultiplyAssign();
  void ElementPrimitiveMultiplyAssign();
  void ElementRightMultiplyAssign();
  void ElementObjectMultiplyAssign();
  void PrimitiveDivide();
  void LeftDivide();
  void RightDivide();
  void ObjectDivide();
  void VariablePrimitiveDivideAssign();
  void VariableRightDivideAssign();
  void VariableObjectDivideAssign();
  void ElementPrimitiveDivideAssign();
  void ElementRightDivideAssign();
  void ElementObjectDivideAssign();

  friend class Object;
  friend class Module;
};

}  // namespace vm
}  // namespace fetch
