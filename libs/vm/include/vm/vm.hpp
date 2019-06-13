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
#include "vm/generator.hpp"
#include "vm/io_observer_interface.hpp"
#include "vm/string.hpp"
#include "vm/variant.hpp"
#include <cassert>
#include <iostream>
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
    bool success{true};

    success &= AddSingle(std::forward<T>(parameter));
    success &= Add(std::forward<Args>(args)...);

    return success;
  }

  bool AddSingle(Variant parameter)
  {
    // TODO: Probably should make a deep copy

    params_.push_back(std::move(parameter));
    return true;
  }

  template <typename T>
  IfIsPrimitive<T, bool> AddSingle(T &&parameter)
  {
    return AddInternal(std::forward<T>(parameter));
  }

  template <typename T>
  IfIsPtr<T, bool> AddSingle(T &&obj)
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
  using InputDeviceMap  = std::unordered_map<std::string, std::istream *>;
  using OutputDeviceMap = std::unordered_map<std::string, std::ostream *>;

  explicit VM(Module *module);
  ~VM() = default;

  static constexpr char const *const STDOUT = "stdout";

  RegisteredTypes const &registered_types() const
  {
    return registered_types_;
  }

  bool GenerateExecutable(IR const &ir, std::string const &name, Executable &executable,
                          std::vector<std::string> &errors);

  template <typename... Ts>
  bool Execute(Executable const &executable, std::string const &name, std::string &error,
               Variant &output, Ts const &... parameters)

  {
    ParameterPack parameter_pack{registered_types_};

    if (!parameter_pack.Add(parameters...))
    {
      error = "Unable to generate parameter pack";
      return false;
    }

    return Execute(executable, name, error, output, parameter_pack);
  }

  bool Execute(Executable const &executable, std::string const &name, std::string &error,
               Variant &output, ParameterPack const &parameters)
  {
    bool success{false};

    Executable::Function const *f = executable.FindFunction(name);
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

        executable_ = &executable;
        function_   = f;

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

    return success;
  }

  std::string GetUniqueId(TypeId type_id) const
  {
    auto info = GetTypeInfo(type_id);
    return info.name;
  }

  template <typename T>
  TypeId GetTypeId()
  {
    return registered_types_.GetTypeId(TypeIndex(typeid(T)));
  }

  template <typename T, typename... Ts>
  Ptr<T> CreateNewObject(Ts &&... args)
  {
    return new T(this, GetTypeId<T>(), std::forward<Ts>(args)...);
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

  std::ostream &GetOutputDevice(std::string const &name)
  {
    if (output_devices_.find(name) == output_devices_.end())
    {
      RuntimeError("output device " + name + " does not exist.");
      return std::cout;
    }
    return *output_devices_[name];
  }

  std::istream &GetInputDevice(std::string const &name)
  {
    if (input_devices_.find(name) == input_devices_.end())
    {
      RuntimeError("input device " + name + " does not exist.");
      return std::cin;
    }
    return *input_devices_[name];
  }

  void DetachInputDevice(std::string const &name)
  {
    auto it = input_devices_.find(name);
    if (it != input_devices_.end())
    {
      input_devices_.erase(it);
    }
    else
    {
      throw std::runtime_error("Input device does not exists.");
    }
  }

  void AttachInputDevice(std::string name, std::istream &device)
  {
    if (input_devices_.find(name) != input_devices_.end())
    {
      throw std::runtime_error("Input device already exists.");
    }

    input_devices_.insert({std::move(name), &device});
  }

  void DetachOutputDevice(std::string const &name)
  {
    auto it = output_devices_.find(name);
    if (it != output_devices_.end())
    {
      output_devices_.erase(it);
    }
    else
    {
      throw std::runtime_error("Output device does not exists.");
    }
  }

  void AttachOutputDevice(std::string name, std::ostream &device)
  {
    if (output_devices_.find(name) != output_devices_.end())
    {
      throw std::runtime_error("output device already exists.");
    }

    output_devices_.insert({std::move(name), &device});
  }

  void AddOutputLine(std::string const &line)
  {
    output_buffer_ << line << '\n';
  }

  // These two are public for the benefit of the static Constructor() functions in
  // each of the Object-derived classes
  void RuntimeError(std::string const &message);
  bool HasError() const
  {
    return !error_.empty();
  }
  TypeInfo const &GetTypeInfo(TypeId type_id) const
  {
    return type_info_array_[type_id];
  }

  bool IsDefaultConstructable(TypeId type_id) const
  {
    TypeIndex idx = registered_types_.GetTypeIndex(type_id);
    auto it = deserialization_constructors_.find(idx);
    return (it != deserialization_constructors_.end());
  }

  Ptr< Object > DefaultConstruct(TypeId type_id) 
  {
    TypeIndex idx = registered_types_.GetTypeIndex(type_id);
    auto it = deserialization_constructors_.find(idx);

    if(it == deserialization_constructors_.end())
    {
      RuntimeError("object is not default constructible.");
      return nullptr;
    }

    auto &constructor = it->second;
    return constructor(this, type_id);
  }

private:
  static const int FRAME_STACK_SIZE = 50;
  static const int STACK_SIZE       = 5000;
  static const int MAX_LIVE_OBJECTS = 200;
  static const int MAX_RANGE_LOOPS  = 50;

  struct OpcodeInfo
  {
    OpcodeInfo() = default;
    OpcodeInfo(std::string name__, Handler handler__)
      : name(std::move(name__))
      , handler(std::move(handler__))
    {}

    std::string name;
    Handler     handler;
  };
  using OpcodeInfoArray = std::vector<OpcodeInfo>;
  using OpcodeMap       = std::unordered_map<std::string, uint16_t>;

  struct Frame
  {
    Executable::Function const *function;
    int                         bsp;
    uint16_t                    pc;
  };

  struct ForRangeLoop
  {
    uint16_t  variable_index;
    Primitive current;
    Primitive target;
    Primitive delta;
  };

  struct LiveObjectInfo
  {
    int      frame_sp;
    uint16_t variable_index;
    uint16_t scope_number;
  };

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
  template <typename Type, typename ReturnType, typename MemberFunction, typename... Ts>
  friend struct MemberFunctionInvokerHelper;
  template <typename Type, typename ReturnType, typename Constructor, typename... Ts>
  friend struct ConstructorInvokerHelper;
  template <typename ReturnType, typename StaticMemberFunction, typename... Ts>
  friend struct StaticMemberFunctionInvokerHelper;
  template <typename ReturnType, typename Functor, typename... Ts>
  friend struct FunctorInvokerHelper;

  TypeInfoArray                  type_info_array_;
  TypeInfoMap                    type_info_map_;
  RegisteredTypes                registered_types_;
  OpcodeInfoArray                opcode_info_array_;
  OpcodeMap                      opcode_map_;
  Generator                      generator_;
  Executable const *             executable_;
  Executable::Function const *   function_;
  std::vector<Ptr<String>>       strings_;
  Frame                          frame_stack_[FRAME_STACK_SIZE];
  int                            frame_sp_;
  int                            bsp_;
  Variant                        stack_[STACK_SIZE];
  int                            sp_;
  ForRangeLoop                   range_loop_stack_[MAX_RANGE_LOOPS];
  int                            range_loop_sp_;
  LiveObjectInfo                 live_object_stack_[MAX_LIVE_OBJECTS];
  int                            live_object_sp_;
  uint16_t                       pc_;
  uint16_t                       instruction_pc_;
  Executable::Instruction const *instruction_;
  bool                           stop_;
  std::string                    error_;
  std::ostringstream             output_buffer_;
  IoObserverInterface *          io_observer_{nullptr};
  OutputDeviceMap                output_devices_;
  InputDeviceMap                 input_devices_;
  DeserializeConstructorMap      deserialization_constructors_;

  void AddOpcodeInfo(uint16_t opcode, std::string const &name, Handler const &handler)
  {
    opcode_info_array_[opcode] = OpcodeInfo(name, handler);
  }

  bool Execute(std::string &error, Variant &output);
  void Destruct(uint16_t scope_number);

  TypeId FindType(std::string const &name) const
  {
    auto it = type_info_map_.find(name);
    if (it != type_info_map_.end())
    {
      return it->second;
    }
    return TypeIds::Unknown;
  }

  uint16_t FindOpcode(std::string const &name) const
  {
    auto it = opcode_map_.find(name);
    if (it != opcode_map_.end())
    {
      return it->second;
    }
    return Opcodes::Unknown;
  }

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

  Variant &GetVariable(uint16_t variable_index)
  {
    return stack_[bsp_ + variable_index];
  }

  struct PrimitiveEqual
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsEqual(lhs, rhs), TypeIds::Bool);
    }
  };

  struct PrimitiveNotEqual
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

  struct PrimitiveLessThan
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsLessThan(lhs, rhs), TypeIds::Bool);
    }
  };

  struct ObjectLessThan
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.Assign(lhsv.object->IsLessThan(lhsv.object, rhsv.object), TypeIds::Bool);
    }
  };

  struct PrimitiveLessThanOrEqual
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsLessThanOrEqual(lhs, rhs), TypeIds::Bool);
    }
  };

  struct ObjectLessThanOrEqual
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.Assign(lhsv.object->IsLessThanOrEqual(lhsv.object, rhsv.object), TypeIds::Bool);
    }
  };

  struct PrimitiveGreaterThan
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsGreaterThan(lhs, rhs), TypeIds::Bool);
    }
  };

  struct ObjectGreaterThan
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.Assign(lhsv.object->IsGreaterThan(lhsv.object, rhsv.object), TypeIds::Bool);
    }
  };

  struct PrimitiveGreaterThanOrEqual
  {
    template <typename T>
    static void Apply(Variant &lhsv, T &lhs, T &rhs)
    {
      lhsv.Assign(math::IsGreaterThanOrEqual(lhs, rhs), TypeIds::Bool);
    }
  };

  struct ObjectGreaterThanOrEqual
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.Assign(lhsv.object->IsGreaterThanOrEqual(lhsv.object, rhsv.object), TypeIds::Bool);
    }
  };

  struct PrefixInc
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      rhs = ++lhs;
    }
  };

  struct PrefixDec
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      rhs = --lhs;
    }
  };

  struct PostfixInc
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      rhs = lhs++;
    }
  };

  struct PostfixDec
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      rhs = lhs--;
    }
  };

  struct Inc
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T & /* rhs */)
    {
      ++lhs;
    }
  };

  struct Dec
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T & /* rhs */)
    {
      --lhs;
    }
  };

  struct PrimitiveNegate
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T & /* rhs */)
    {
      lhs = T(-lhs);
    }
  };

  struct PrimitiveAdd
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      lhs = T(lhs + rhs);
    }
  };

  struct ObjectAdd
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->Add(lhso, rhso);
    }
  };

  struct ObjectLeftAdd
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftAdd(lhsv, rhsv);
    }
  };

  struct ObjectRightAdd
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightAdd(lhsv, rhsv);
    }
  };

  struct ObjectInplaceAdd
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->InplaceAdd(lhso, rhso);
    }
  };

  struct ObjectInplaceRightAdd
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->InplaceRightAdd(lhso, rhsv);
    }
  };

  struct PrimitiveSubtract
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      lhs = T(lhs - rhs);
    }
  };

  struct ObjectSubtract
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->Subtract(lhso, rhso);
    }
  };

  struct ObjectLeftSubtract
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftSubtract(lhsv, rhsv);
    }
  };

  struct ObjectRightSubtract
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightSubtract(lhsv, rhsv);
    }
  };

  struct ObjectInplaceSubtract
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->InplaceSubtract(lhso, rhso);
    }
  };

  struct ObjectInplaceRightSubtract
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->InplaceRightSubtract(lhso, rhsv);
    }
  };

  struct PrimitiveMultiply
  {
    template <typename T>
    static void Apply(VM * /* vm */, T &lhs, T &rhs)
    {
      lhs = T(lhs * rhs);
    }
  };

  struct ObjectMultiply
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->Multiply(lhso, rhso);
    }
  };

  struct ObjectLeftMultiply
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftMultiply(lhsv, rhsv);
    }
  };

  struct ObjectRightMultiply
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightMultiply(lhsv, rhsv);
    }
  };

  struct ObjectInplaceMultiply
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->InplaceMultiply(lhso, rhso);
    }
  };

  struct ObjectInplaceRightMultiply
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->InplaceRightMultiply(lhso, rhsv);
    }
  };

  struct PrimitiveDivide
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

  struct ObjectDivide
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->Divide(lhso, rhso);
    }
  };

  struct ObjectLeftDivide
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftDivide(lhsv, rhsv);
    }
  };

  struct ObjectRightDivide
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightDivide(lhsv, rhsv);
    }
  };

  struct ObjectInplaceDivide
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->InplaceDivide(lhso, rhso);
    }
  };

  struct ObjectInplaceRightDivide
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->InplaceRightDivide(lhso, rhsv);
    }
  };

  struct PrimitiveModulo
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

  template <typename Op>
  void ExecutePrimitiveRelationalOp(TypeId type_id, Variant &lhsv, Variant &rhsv)
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
    case TypeIds::UInt8:
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
  void ExecuteIntegralOp(TypeId type_id, Variant &lhsv, Variant &rhsv)
  {
    switch (type_id)
    {
    case TypeIds::Int8:
    {
      Op::Apply(this, lhsv.primitive.i8, rhsv.primitive.i8);
      break;
    }
    case TypeIds::UInt8:
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
  void ExecuteNumericOp(TypeId type_id, Variant &lhsv, Variant &rhsv)
  {
    switch (type_id)
    {
    case TypeIds::Int8:
    {
      Op::Apply(this, lhsv.primitive.i8, rhsv.primitive.i8);
      break;
    }
    case TypeIds::UInt8:
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
  void ExecuteIntegralInplaceOp(TypeId type_id, void *lhs, Variant &rhsv)
  {
    switch (type_id)
    {
    case TypeIds::Int8:
    {
      Op::Apply(this, *static_cast<int8_t *>(lhs), rhsv.primitive.i8);
      break;
    }
    case TypeIds::UInt8:
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
  void ExecuteNumericInplaceOp(TypeId type_id, void *lhs, Variant &rhsv)
  {
    switch (type_id)
    {
    case TypeIds::Int8:
    {
      Op::Apply(this, *static_cast<int8_t *>(lhs), rhsv.primitive.i8);
      break;
    }
    case TypeIds::UInt8:
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
  void DoPrimitiveRelationalOp()
  {
    Variant &rhsv = Pop();
    Variant &lhsv = Top();
    ExecutePrimitiveRelationalOp<Op>(instruction_->type_id, lhsv, rhsv);
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
  void DoPrefixPostfixOp(TypeId type_id, void *lhs)
  {
    Variant &rhsv = Push();
    ExecuteIntegralInplaceOp<Op>(type_id, lhs, rhsv);
    rhsv.type_id = instruction_->type_id;
  }

  template <typename Op>
  void DoVariablePrefixPostfixOp()
  {
    Variant &variable = GetVariable(instruction_->index);
    DoPrefixPostfixOp<Op>(instruction_->type_id, &variable.primitive);
  }

  template <typename Op>
  void DoIntegralOp()
  {
    Variant &rhsv = Pop();
    Variant &lhsv = Top();
    ExecuteIntegralOp<Op>(instruction_->type_id, lhsv, rhsv);
    rhsv.Reset();
  }

  template <typename Op>
  void DoNumericOp()
  {
    Variant &rhsv = Pop();
    Variant &lhsv = Top();
    ExecuteNumericOp<Op>(instruction_->type_id, lhsv, rhsv);
    rhsv.Reset();
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
  void DoObjectLeftOp()
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
  void DoObjectRightOp()
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
  void DoIntegralInplaceOp(TypeId type_id, void *lhs)
  {
    Variant &rhsv = Pop();
    ExecuteIntegralInplaceOp<Op>(type_id, lhs, rhsv);
    rhsv.Reset();
  }

  template <typename Op>
  void DoNumericInplaceOp(TypeId type_id, void *lhs)
  {
    Variant &rhsv = Pop();
    ExecuteNumericInplaceOp<Op>(type_id, lhs, rhsv);
    rhsv.Reset();
  }

  template <typename Op>
  void DoObjectInplaceOp(Ptr<Object> &lhso)
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
  void DoObjectInplaceRightOp(Ptr<Object> &lhso)
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
  void DoVariableIntegralInplaceOp()
  {
    Variant &variable = GetVariable(instruction_->index);
    DoIntegralInplaceOp<Op>(instruction_->type_id, &variable.primitive);
  }

  template <typename Op>
  void DoVariableNumericInplaceOp()
  {
    Variant &variable = GetVariable(instruction_->index);
    DoNumericInplaceOp<Op>(instruction_->type_id, &variable.primitive);
  }

  template <typename Op>
  void DoVariableObjectInplaceOp()
  {
    Variant &variable = GetVariable(instruction_->index);
    DoObjectInplaceOp<Op>(variable.object);
  }

  template <typename Op>
  void DoVariableObjectInplaceRightOp()
  {
    Variant &variable = GetVariable(instruction_->index);
    DoObjectInplaceRightOp<Op>(variable.object);
  }

  //
  // Opcode handler prototypes
  //

  void Handler__VariableDeclare();
  void Handler__VariableDeclareAssign();
  void Handler__PushNull();
  void Handler__PushFalse();
  void Handler__PushTrue();
  void Handler__PushString();
  void Handler__PushConstant();
  void Handler__PushVariable();
  void Handler__PopToVariable();
  void Handler__Inc();
  void Handler__Dec();
  void Handler__Duplicate();
  void Handler__DuplicateInsert();
  void Handler__Discard();
  void Handler__Destruct();
  void Handler__Break();
  void Handler__Continue();
  void Handler__Jump();
  void Handler__JumpIfFalse();
  void Handler__JumpIfTrue();
  void Handler__Return();
  void Handler__ForRangeInit();
  void Handler__ForRangeIterate();
  void Handler__ForRangeTerminate();
  void Handler__InvokeUserDefinedFreeFunction();
  void Handler__VariablePrefixInc();
  void Handler__VariablePrefixDec();
  void Handler__VariablePostfixInc();
  void Handler__VariablePostfixDec();
  void Handler__And();
  void Handler__Or();
  void Handler__Not();
  void Handler__PrimitiveEqual();
  void Handler__ObjectEqual();
  void Handler__PrimitiveNotEqual();
  void Handler__ObjectNotEqual();
  void Handler__PrimitiveLessThan();
  void Handler__ObjectLessThan();
  void Handler__PrimitiveLessThanOrEqual();
  void Handler__ObjectLessThanOrEqual();
  void Handler__PrimitiveGreaterThan();
  void Handler__ObjectGreaterThan();
  void Handler__PrimitiveGreaterThanOrEqual();
  void Handler__ObjectGreaterThanOrEqual();
  void Handler__PrimitiveNegate();
  void Handler__ObjectNegate();
  void Handler__PrimitiveAdd();
  void Handler__ObjectAdd();
  void Handler__ObjectLeftAdd();
  void Handler__ObjectRightAdd();
  void Handler__VariablePrimitiveInplaceAdd();
  void Handler__VariableObjectInplaceAdd();
  void Handler__VariableObjectInplaceRightAdd();
  void Handler__PrimitiveSubtract();
  void Handler__ObjectSubtract();
  void Handler__ObjectLeftSubtract();
  void Handler__ObjectRightSubtract();
  void Handler__VariablePrimitiveInplaceSubtract();
  void Handler__VariableObjectInplaceSubtract();
  void Handler__VariableObjectInplaceRightSubtract();
  void Handler__PrimitiveMultiply();
  void Handler__ObjectMultiply();
  void Handler__ObjectLeftMultiply();
  void Handler__ObjectRightMultiply();
  void Handler__VariablePrimitiveInplaceMultiply();
  void Handler__VariableObjectInplaceMultiply();
  void Handler__VariableObjectInplaceRightMultiply();
  void Handler__PrimitiveDivide();
  void Handler__ObjectDivide();
  void Handler__ObjectLeftDivide();
  void Handler__ObjectRightDivide();
  void Handler__VariablePrimitiveInplaceDivide();
  void Handler__VariableObjectInplaceDivide();
  void Handler__VariableObjectInplaceRightDivide();
  void Handler__PrimitiveModulo();
  void Handler__VariablePrimitiveInplaceModulo();

  friend class Object;
  friend class Module;
  friend class Generator;
};

}  // namespace vm
}  // namespace fetch
