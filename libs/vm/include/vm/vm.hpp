#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "vm/common.hpp"
#include "vm/generator.hpp"
#include "vm/object.hpp"
#include "vm/opcodes.hpp"
#include "vm/string.hpp"
#include "vm/user_defined_object.hpp"
#include "vm/variant.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fetch {
namespace vm {

template <typename ArgsTuple, typename... Args>
bool EstimateCharge(VM *vm, ChargeEstimator<Args...> &&e, ArgsTuple const &args);

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
    using ManagedType = GetManagedType<std::decay_t<T>>;
    return types.GetTypeId(TypeIndex(typeid(ManagedType)));
  }
};
template <typename T>
struct Getter<T, std::enable_if_t<IsVariant<T>>>
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
class IR;
class IoObserverInterface;
class Module;

class VM;
class ParameterPack
{
public:
  // Construction / Destruction
  explicit ParameterPack(RegisteredTypes const &registered_types, VariantArray params = {},
                         VM *vm = nullptr)
    : registered_types_{registered_types}
    , params_{std::move(params)}
    , vm_{vm}
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
    // TODO(1669): Probably should make a deep copy

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

  // Implementation is at the bottom of file
  // due to dependency on VM
  template <typename T>
  IfIsExternal<T, bool> AddSingle(T val);

  bool Add()
  {
    return true;
  }

  // Operators
  ParameterPack &operator=(ParameterPack const &) = delete;
  ParameterPack &operator=(ParameterPack &&) = delete;

private:
  template <typename T>
  bool AddInternal(T const &value)
  {
    bool         success{false};
    TypeId const type_id = Getter<T>::GetTypeId(registered_types_, value);

    if (TypeIds::Unknown != type_id)
    {
      // add the value to the map
      params_.emplace_back(value, type_id);

      // signal great success
      success = true;
    }

    return success;
  }

  bool AddInternal(Ptr<Object> const &value)
  {
    // add the value to the map
    // TODO(tfr): Check ownership of Ptr.
    Variant v(value, value->GetTypeId());
    params_.emplace_back(std::move(v));

    return true;
  }

  RegisteredTypes const &registered_types_;
  VariantArray           params_;
  VM *const              vm_;
};

using ContractInvocationHandler = std::function<bool(
    VM * /* vm */, std::string const & /* identity */, Executable::Contract const & /* contract */,
    Executable::Function const & /* function */, VariantArray /* parameters */,
    std::string & /* error */, Variant & /* output */)>;

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
    if (f != nullptr)
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
            error = "mismatched parameters: expected argument " + std::to_string(i);
            error += "to be of type " + GetTypeName(f->variables[i].type_id) + " but got ";
            error += GetTypeName(parameter.type_id);
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

        if (executable_ != nullptr)
        {
          error = "Executable already loaded. Please unload first.";
        }
        else
        {
          LoadExecutable(&executable);
          function_ = f;

          // execute the function
          success = Execute(error, output);
          UnloadExecutable();
        }
      }
      else
      {
        error = "mismatched parameters: expected " + std::to_string(num_parameters) + " arguments";
        error += ", but got " + std::to_string(parameters.size());
      }
    }
    else
    {
      error = "unable to find function '" + name + "'";
    }

    return success;
  }

  std::string GetTypeName(TypeId type_id) const
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
    return Ptr<T>{new T(this, GetTypeId<T>(), std::forward<Ts>(args)...)};
  }

  void SetContractInvocationHandler(ContractInvocationHandler handler)
  {
    contract_invocation_handler_ = std::move(handler);
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
      RuntimeError("Output device " + name + " does not exist.");
      return std::cout;
    }
    return *output_devices_[name];
  }

  std::istream &GetInputDevice(std::string const &name)
  {
    if (input_devices_.find(name) == input_devices_.end())
    {
      RuntimeError("Input device " + name + " does not exist.");
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
      throw std::runtime_error("Output device already exists.");
    }

    output_devices_.insert({std::move(name), &device});
  }

  void AddOutputLine(std::string const &line)
  {
    output_buffer_ << line << '\n';
  }

  void RuntimeError(std::string const &message);
  bool HasError() const
  {
    return !error_.empty();
  }

  Executable::UserDefinedType const &GetUserDefinedType(uint16_t type_id) const
  {
    return executable_->GetUserDefinedType(type_id);
  }

  void LoadExecutable(Executable const *executable)
  {
    executable_                   = executable;
    std::size_t const num_strings = executable_->strings.size();
    strings_                      = std::vector<Ptr<String>>(num_strings);
    for (std::size_t i = 0; i < num_strings; ++i)
    {
      std::string const &str = executable_->strings[i];
      strings_[i]            = Ptr<String>(new String(this, str));
    }

    std::size_t const num_local_types = executable_->types.size();
    for (std::size_t i = 0; i < num_local_types; ++i)
    {
      TypeInfo const &type_info = executable_->types[i];
      type_info_array_.push_back(type_info);
    }
  }

  void UnloadExecutable()
  {
    strings_.clear();

    std::size_t const num_local_types = executable_->types.size();
    for (std::size_t i = 0; i < num_local_types; ++i)
    {
      type_info_array_.pop_back();
    }

    executable_ = nullptr;
  }

  TypeInfo const &GetTypeInfo(TypeId type_id) const
  {
    return type_info_array_[type_id];
  }

  bool IsDefaultSerializeConstructable(TypeId type_id)
  {
    TypeIndex idx = registered_types_.GetTypeIndex(type_id);
    auto      it  = deserialization_constructors_.find(idx);

    if (it == deserialization_constructors_.end())
    {

      TypeInfo tinfo = GetTypeInfo(type_id);
      if (tinfo.template_type_id == TypeIds::Unknown)
      {
        return false;
      }

      idx = registered_types_.GetTypeIndex(tinfo.template_type_id);
      it  = deserialization_constructors_.find(idx);

      return (it != deserialization_constructors_.end());
    }

    return true;
  }

  Ptr<Object> DefaultSerializeConstruct(TypeId type_id)
  {
    // Resolving constructor
    TypeIndex idx = registered_types_.GetTypeIndex(type_id);
    auto      it  = deserialization_constructors_.find(idx);

    if (it == deserialization_constructors_.end())
    {
      // Testing if there is an interface constructor
      TypeInfo tinfo = GetTypeInfo(type_id);
      if (tinfo.template_type_id != TypeIds::Unknown)
      {
        idx = registered_types_.GetTypeIndex(tinfo.template_type_id);
        it  = deserialization_constructors_.find(idx);
      }
    }

    if (it == deserialization_constructors_.end())
    {
      RuntimeError("object is not default constructible.");
      return {};
    }

    auto &constructor = it->second;
    auto  ret         = constructor(this, type_id);

    return ret;
  }

  template <typename T>
  bool HasCPPCopyConstructor()
  {
    auto type_index = TypeIndex(typeid(T));
    return (cpp_copy_constructors_.find(type_index) != cpp_copy_constructors_.end());
  }

  template <typename T>
  Ptr<Object> CPPCopyConstruct(T const &val)
  {
    auto it = cpp_copy_constructors_.find(TypeIndex(typeid(T)));
    if (it == cpp_copy_constructors_.end())
    {
      return {};
    }

    return it->second(this, static_cast<void const *>(&val));
  }

  struct OpcodeInfo
  {
    OpcodeInfo() = default;
    OpcodeInfo(std::string unique_name__, Handler handler__, ChargeAmount static_charge__)
      : unique_name(std::move(unique_name__))
      , handler(std::move(handler__))
      , static_charge{static_charge__}
    {}

    std::string  unique_name;
    Handler      handler;
    ChargeAmount static_charge{};
  };

  ChargeAmount GetChargeTotal() const;
  void         IncreaseChargeTotal(ChargeAmount amount);
  ChargeAmount GetChargeLimit() const;
  bool         ChargeLimitExceeded();
  void         SetChargeLimit(ChargeAmount limit);

  void UpdateCharges(std::unordered_map<std::string, ChargeAmount> const &opcode_static_charges);

private:
  static const int FRAME_STACK_SIZE = 50;
  static const int STACK_SIZE       = 1024;
  static const int MAX_RANGE_LOOPS  = 16;

  using OpcodeInfoArray = std::vector<OpcodeInfo>;
  using OpcodeMap       = std::unordered_map<std::string, uint16_t>;

  struct Frame
  {
    Executable::Function const *function{};
    int                         bsp{};
    uint16_t                    pc{};
    Variant                     self;
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
    LiveObjectInfo(int frame_sp__, uint16_t variable_index__, uint16_t scope_number__)
      : frame_sp(frame_sp__)
      , variable_index(variable_index__)
      , scope_number(scope_number__)
    {}
    int      frame_sp{};
    uint16_t variable_index{};
    uint16_t scope_number{};
  };

  template <typename T>
  friend struct StackGetter;
  template <typename T>
  friend struct StackSetter;
  template <typename T, typename S>
  friend struct TypeGetter;
  template <typename T, typename S>
  friend struct ParameterTypeGetter;
  template <int, typename, typename, typename>
  friend struct VmFreeFunctionInvoker;
  template <int, typename, typename, typename>
  friend struct VmMemberFunctionInvoker;

  TypeInfoArray                  type_info_array_;
  TypeInfoMap                    type_info_map_;
  RegisteredTypes                registered_types_;
  OpcodeInfoArray                opcode_info_array_;
  OpcodeMap                      opcode_map_;
  Generator                      generator_;
  Executable const *             executable_{};
  Executable::Function const *   function_{};
  std::vector<Ptr<String>>       strings_;
  Frame                          frame_stack_[FRAME_STACK_SIZE]{};
  int                            frame_sp_{};
  int                            bsp_{};
  Variant                        stack_[STACK_SIZE];
  int                            sp_{};
  ForRangeLoop                   range_loop_stack_[MAX_RANGE_LOOPS]{};
  int                            range_loop_sp_{};
  std::vector<LiveObjectInfo>    live_object_stack_;
  uint16_t                       pc_{};
  Variant                        self_;
  uint16_t                       instruction_pc_{};
  Executable::Instruction const *instruction_{};
  bool                           stop_{};
  std::string                    error_;
  ContractInvocationHandler      contract_invocation_handler_{};
  std::ostringstream             output_buffer_;
  IoObserverInterface *          io_observer_{};
  OutputDeviceMap                output_devices_;
  InputDeviceMap                 input_devices_;
  DeserializeConstructorMap      deserialization_constructors_;
  CPPCopyConstructorMap          cpp_copy_constructors_;
  OpcodeInfo *                   current_op_{};

  /// @name Charges
  /// @{
  ChargeAmount charge_limit_{std::numeric_limits<ChargeAmount>::max()};
  ChargeAmount charge_total_{0};
  /// @}

  void AddOpcodeInfo(uint16_t opcode, std::string unique_name, Handler handler,
                     ChargeAmount static_charge = 1)
  {
    opcode_info_array_[opcode] =
        OpcodeInfo(std::move(unique_name), std::move(handler), static_charge);
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

  bool PushFrame()
  {
    if (++frame_sp_ < FRAME_STACK_SIZE)
    {
      Frame &frame   = frame_stack_[frame_sp_];
      frame.function = function_;
      frame.bsp      = bsp_;
      frame.pc       = pc_;
      frame.self     = self_;
      return true;
    }
    --frame_sp_;
    RuntimeError("frame stack overflow");
    return false;
  }

  void PopFrame()
  {
    Frame &frame = frame_stack_[frame_sp_--];
    function_    = frame.function;
    bsp_         = frame.bsp;
    pc_          = frame.pc;
    self_        = std::move(frame.self);
  }

  Variant &Pop()
  {
    return stack_[sp_--];
  }

  Variant &Top()
  {
    return stack_[sp_];
  }

  Variant &GetLocalVariable(uint16_t variable_index)
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

  bool IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
  {
    if (lhso)
    {
      if (rhso)
      {
        if (EstimateCharge(this, ChargeEstimator<>([lhso, rhso]() -> ChargeAmount {
                             return lhso->IsEqualChargeEstimator(lhso, rhso);
                           }),
                           std::tuple<>{}))
        {
          return lhso->IsEqual(lhso, rhso);
        }
      }
      return false;
    }
    return (rhso == nullptr);
  }

  bool IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
  {
    if (lhso)
    {
      if (rhso)
      {
        if (EstimateCharge(this, ChargeEstimator<>([lhso, rhso]() -> ChargeAmount {
                             return lhso->IsNotEqualChargeEstimator(lhso, rhso);
                           }),
                           std::tuple<>{}))
        {
          return lhso->IsNotEqual(lhso, rhso);
        }
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

    static ChargeAmount ApplyChargeEstimator(Variant const &lhsv, Variant const &rhsv)
    {
      return lhsv.object->IsLessThanChargeEstimator(lhsv.object, rhsv.object);
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

    static ChargeAmount ApplyChargeEstimator(Variant const &lhsv, Variant const &rhsv)
    {
      return lhsv.object->IsLessThanOrEqualChargeEstimator(lhsv.object, rhsv.object);
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

    static ChargeAmount ApplyChargeEstimator(Variant const &lhsv, Variant const &rhsv)
    {
      return lhsv.object->IsGreaterThanChargeEstimator(lhsv.object, rhsv.object);
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

    static ChargeAmount ApplyChargeEstimator(Variant const &lhsv, Variant const &rhsv)
    {
      return lhsv.object->IsGreaterThanOrEqualChargeEstimator(lhsv.object, rhsv.object);
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

    static ChargeAmount ApplyChargeEstimator(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
    {
      return lhso->AddChargeEstimator(lhso, rhso);
    }
  };

  struct ObjectLeftAdd
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftAdd(lhsv, rhsv);
    }

    static ChargeAmount ApplyChargeEstimator(Variant const &lhsv, Variant const &rhsv)
    {
      return rhsv.object->LeftAddChargeEstimator(lhsv, rhsv);
    }
  };

  struct ObjectRightAdd
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightAdd(lhsv, rhsv);
    }

    static ChargeAmount ApplyChargeEstimator(Variant const &lhsv, Variant const &rhsv)
    {
      return lhsv.object->RightAddChargeEstimator(lhsv, rhsv);
    }
  };

  struct ObjectInplaceAdd
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->InplaceAdd(lhso, rhso);
    }

    static ChargeAmount ApplyChargeEstimator(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
    {
      return lhso->InplaceAddChargeEstimator(lhso, rhso);
    }
  };

  struct ObjectInplaceRightAdd
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->InplaceRightAdd(lhso, rhsv);
    }

    static ChargeAmount ApplyChargeEstimator(Ptr<Object> const &lhso, Variant const &rhsv)
    {
      return lhso->InplaceRightAddChargeEstimator(lhso, rhsv);
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

    static ChargeAmount ApplyChargeEstimator(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
    {
      return lhso->SubtractChargeEstimator(lhso, rhso);
    }
  };

  struct ObjectLeftSubtract
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftSubtract(lhsv, rhsv);
    }

    static ChargeAmount ApplyChargeEstimator(Variant const &lhsv, Variant const &rhsv)
    {
      return rhsv.object->LeftSubtractChargeEstimator(lhsv, rhsv);
    }
  };

  struct ObjectRightSubtract
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightSubtract(lhsv, rhsv);
    }

    static ChargeAmount ApplyChargeEstimator(Variant const &lhsv, Variant const &rhsv)
    {
      return lhsv.object->RightSubtractChargeEstimator(lhsv, rhsv);
    }
  };

  struct ObjectInplaceSubtract
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->InplaceSubtract(lhso, rhso);
    }

    static ChargeAmount ApplyChargeEstimator(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
    {
      return lhso->InplaceSubtractChargeEstimator(lhso, rhso);
    }
  };

  struct ObjectInplaceRightSubtract
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->InplaceRightSubtract(lhso, rhsv);
    }

    static ChargeAmount ApplyChargeEstimator(Ptr<Object> const &lhso, Variant const &rhsv)
    {
      return lhso->InplaceRightSubtractChargeEstimator(lhso, rhsv);
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

    static ChargeAmount ApplyChargeEstimator(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
    {
      return lhso->MultiplyChargeEstimator(lhso, rhso);
    }
  };

  struct ObjectLeftMultiply
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftMultiply(lhsv, rhsv);
    }

    static ChargeAmount ApplyChargeEstimator(Variant const &lhsv, Variant const &rhsv)
    {
      return rhsv.object->LeftMultiplyChargeEstimator(lhsv, rhsv);
    }
  };

  struct ObjectRightMultiply
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightMultiply(lhsv, rhsv);
    }

    static ChargeAmount ApplyChargeEstimator(Variant const &lhsv, Variant const &rhsv)
    {
      return lhsv.object->RightMultiplyChargeEstimator(lhsv, rhsv);
    }
  };

  struct ObjectInplaceMultiply
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->InplaceMultiply(lhso, rhso);
    }

    static ChargeAmount ApplyChargeEstimator(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
    {
      return lhso->InplaceMultiplyChargeEstimator(lhso, rhso);
    }
  };

  struct ObjectInplaceRightMultiply
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->InplaceRightMultiply(lhso, rhsv);
    }

    static ChargeAmount ApplyChargeEstimator(Ptr<Object> const &lhso, Variant const &rhsv)
    {
      return lhso->InplaceRightMultiplyChargeEstimator(lhso, rhsv);
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

    static ChargeAmount ApplyChargeEstimator(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
    {
      return lhso->DivideChargeEstimator(lhso, rhso);
    }
  };

  struct ObjectLeftDivide
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      rhsv.object->LeftDivide(lhsv, rhsv);
    }

    static ChargeAmount ApplyChargeEstimator(Variant const &lhsv, Variant const &rhsv)
    {
      return rhsv.object->LeftDivideChargeEstimator(lhsv, rhsv);
    }
  };

  struct ObjectRightDivide
  {
    static void Apply(Variant &lhsv, Variant &rhsv)
    {
      lhsv.object->RightDivide(lhsv, rhsv);
    }

    static ChargeAmount ApplyChargeEstimator(Variant const &lhsv, Variant const &rhsv)
    {
      return lhsv.object->RightDivideChargeEstimator(lhsv, rhsv);
    }
  };

  struct ObjectInplaceDivide
  {
    static void Apply(Ptr<Object> &lhso, Ptr<Object> &rhso)
    {
      lhso->InplaceDivide(lhso, rhso);
    }

    static ChargeAmount ApplyChargeEstimator(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
    {
      return lhso->InplaceDivideChargeEstimator(lhso, rhso);
    }
  };

  struct ObjectInplaceRightDivide
  {
    static void Apply(Ptr<Object> &lhso, Variant &rhsv)
    {
      lhso->InplaceRightDivide(lhso, rhsv);
    }

    static ChargeAmount ApplyChargeEstimator(Ptr<Object> const &lhso, Variant const &rhsv)
    {
      return lhso->InplaceRightDivideChargeEstimator(lhso, rhsv);
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
    case TypeIds::Fixed32:
    {
      fixed_point::fp32_t lhsv_fp32 = fixed_point::fp32_t::FromBase(lhsv.primitive.i32);
      fixed_point::fp32_t rhsv_fp32 = fixed_point::fp32_t::FromBase(rhsv.primitive.i32);
      Op::Apply(lhsv, lhsv_fp32, rhsv_fp32);
      break;
    }
    case TypeIds::Fixed64:
    {
      fixed_point::fp64_t lhsv_fp64 = fixed_point::fp64_t::FromBase(lhsv.primitive.i64);
      fixed_point::fp64_t rhsv_fp64 = fixed_point::fp64_t::FromBase(rhsv.primitive.i64);
      Op::Apply(lhsv, lhsv_fp64, rhsv_fp64);
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
    case TypeIds::Fixed32:
    {
      auto *              lhsv_fp32 = reinterpret_cast<fixed_point::fp32_t *>(&lhsv);
      fixed_point::fp32_t rhsv_fp32 = fixed_point::fp32_t::FromBase(rhsv.primitive.i32);
      Op::Apply(this, *lhsv_fp32, rhsv_fp32);
      break;
    }
    case TypeIds::Fixed64:
    {
      auto *              lhsv_fp64 = reinterpret_cast<fixed_point::fp64_t *>(&lhsv);
      fixed_point::fp64_t rhsv_fp64 = fixed_point::fp64_t::FromBase(rhsv.primitive.i64);
      Op::Apply(this, *lhsv_fp64, rhsv_fp64);
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
    case TypeIds::Fixed32:
    {
      auto *              lhs_fp32  = reinterpret_cast<fixed_point::fp32_t *>(lhs);
      fixed_point::fp32_t rhsv_fp32 = fixed_point::fp32_t::FromBase(rhsv.primitive.i32);
      Op::Apply(this, *lhs_fp32, rhsv_fp32);
      break;
    }
    case TypeIds::Fixed64:
    {
      auto *              lhs_fp64  = reinterpret_cast<fixed_point::fp64_t *>(lhs);
      fixed_point::fp64_t rhsv_fp64 = fixed_point::fp64_t::FromBase(rhsv.primitive.i64);
      Op::Apply(this, *lhs_fp64, rhsv_fp64);
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
      if (EstimateCharge(this, ChargeEstimator<>([lhsv, rhsv]() -> ChargeAmount {
                           return Op::ApplyChargeEstimator(lhsv, rhsv);
                         }),
                         std::tuple<>{}))
      {
        Op::Apply(lhsv, rhsv);
        rhsv.Reset();
      }
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoPrefixPostfixOp(TypeId type_id, void *lhs)
  {
    if (++sp_ < STACK_SIZE)
    {
      Variant &rhsv = Top();
      ExecuteIntegralInplaceOp<Op>(type_id, lhs, rhsv);
      rhsv.type_id = instruction_->type_id;
      return;
    }
    --sp_;
    RuntimeError("stack overflow");
  }

  template <typename Op>
  void DoLocalVariablePrefixPostfixOp()
  {
    Variant &variable = GetLocalVariable(instruction_->index);
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
      if (EstimateCharge(this, ChargeEstimator<>([lhsv, rhsv]() -> ChargeAmount {
                           return Op::ApplyChargeEstimator(lhsv.object, rhsv.object);
                         }),
                         std::tuple<>{}))
      {
        Op::Apply(lhsv.object, rhsv.object);
        rhsv.Reset();
      }
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
      if (!lhsv.IsPrimitive() && !lhsv.object)
      {
        RuntimeError("null reference");
        return;
      }
      if (EstimateCharge(this, ChargeEstimator<>([lhsv, rhsv]() -> ChargeAmount {
                           return Op::ApplyChargeEstimator(lhsv, rhsv);
                         }),
                         std::tuple<>{}))
      {
        Op::Apply(lhsv, rhsv);
        rhsv.Reset();
      }
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
      if (!rhsv.IsPrimitive() && !rhsv.object)
      {
        RuntimeError("null reference");
        return;
      }
      if (EstimateCharge(this, ChargeEstimator<>([lhsv, rhsv]() -> ChargeAmount {
                           return Op::ApplyChargeEstimator(lhsv, rhsv);
                         }),
                         std::tuple<>{}))
      {
        Op::Apply(lhsv, rhsv);
        rhsv.Reset();
      }
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
      if (EstimateCharge(this, ChargeEstimator<>([lhso, rhsv]() -> ChargeAmount {
                           return Op::ApplyChargeEstimator(lhso, rhsv.object);
                         }),
                         std::tuple<>{}))
      {
        Op::Apply(lhso, rhsv.object);
        rhsv.Reset();
      }
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
      if (!rhsv.IsPrimitive() && !rhsv.object)
      {
        RuntimeError("null reference");
        return;
      }
      if (EstimateCharge(this, ChargeEstimator<>([lhso, rhsv]() -> ChargeAmount {
                           return Op::ApplyChargeEstimator(lhso, rhsv);
                         }),
                         std::tuple<>{}))
      {
        Op::Apply(lhso, rhsv);
        rhsv.Reset();
      }
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoLocalVariableIntegralInplaceOp()
  {
    Variant &variable = GetLocalVariable(instruction_->index);
    DoIntegralInplaceOp<Op>(instruction_->type_id, &variable.primitive);
  }

  template <typename Op>
  void DoLocalVariableNumericInplaceOp()
  {
    Variant &variable = GetLocalVariable(instruction_->index);
    DoNumericInplaceOp<Op>(instruction_->type_id, &variable.primitive);
  }

  template <typename Op>
  void DoLocalVariableObjectInplaceOp()
  {
    Variant &variable = GetLocalVariable(instruction_->index);
    DoObjectInplaceOp<Op>(variable.object);
  }

  template <typename Op>
  void DoLocalVariableObjectInplaceRightOp()
  {
    Variant &variable = GetLocalVariable(instruction_->index);
    DoObjectInplaceRightOp<Op>(variable.object);
  }

  template <typename Op>
  void DoMemberVariablePrefixPostfixOp()
  {
    Variant                objectv             = std::move(Pop());
    Ptr<UserDefinedObject> user_defined_object = std::move(objectv.object);
    if (user_defined_object)
    {
      Variant &variable = user_defined_object->GetVariable(instruction_->index);
      DoPrefixPostfixOp<Op>(instruction_->type_id, &variable.primitive);
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoMemberVariableIntegralInplaceOp()
  {
    Variant                objectv             = std::move(stack_[sp_ - 1]);
    Ptr<UserDefinedObject> user_defined_object = std::move(objectv.object);
    if (user_defined_object)
    {
      Variant &variable = user_defined_object->GetVariable(instruction_->index);
      DoIntegralInplaceOp<Op>(instruction_->type_id, &variable.primitive);
      --sp_;
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoMemberVariableNumericInplaceOp()
  {
    Variant                objectv             = std::move(stack_[sp_ - 1]);
    Ptr<UserDefinedObject> user_defined_object = std::move(objectv.object);
    if (user_defined_object)
    {
      Variant &variable = user_defined_object->GetVariable(instruction_->index);
      DoNumericInplaceOp<Op>(instruction_->type_id, &variable.primitive);
      --sp_;
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoMemberVariableObjectInplaceOp()
  {
    Variant                objectv             = std::move(stack_[sp_ - 1]);
    Ptr<UserDefinedObject> user_defined_object = std::move(objectv.object);
    if (user_defined_object)
    {
      Variant &variable = user_defined_object->GetVariable(instruction_->index);
      DoObjectInplaceOp<Op>(variable.object);
      --sp_;
      return;
    }
    RuntimeError("null reference");
  }

  template <typename Op>
  void DoMemberVariableObjectInplaceRightOp()
  {
    Variant                objectv             = std::move(stack_[sp_ - 1]);
    Ptr<UserDefinedObject> user_defined_object = std::move(objectv.object);
    if (user_defined_object)
    {
      Variant &variable = user_defined_object->GetVariable(instruction_->index);
      DoObjectInplaceRightOp<Op>(variable.object);
      --sp_;
      return;
    }
    RuntimeError("null reference");
  }

  //
  // Opcode handler prototypes
  //

  void Handler__LocalVariableDeclare();
  void Handler__LocalVariableDeclareAssign();
  void Handler__PushNull();
  void Handler__PushFalse();
  void Handler__PushTrue();
  void Handler__PushString();
  void Handler__PushConstant();
  void Handler__PushLocalVariable();
  void Handler__PopToLocalVariable();
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
  void Handler__LocalVariablePrefixInc();
  void Handler__LocalVariablePrefixDec();
  void Handler__LocalVariablePostfixInc();
  void Handler__LocalVariablePostfixDec();
  void Handler__JumpIfFalseOrPop();
  void Handler__JumpIfTrueOrPop();
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
  void Handler__LocalVariablePrimitiveInplaceAdd();
  void Handler__LocalVariableObjectInplaceAdd();
  void Handler__LocalVariableObjectInplaceRightAdd();
  void Handler__PrimitiveSubtract();
  void Handler__ObjectSubtract();
  void Handler__ObjectLeftSubtract();
  void Handler__ObjectRightSubtract();
  void Handler__LocalVariablePrimitiveInplaceSubtract();
  void Handler__LocalVariableObjectInplaceSubtract();
  void Handler__LocalVariableObjectInplaceRightSubtract();
  void Handler__PrimitiveMultiply();
  void Handler__ObjectMultiply();
  void Handler__ObjectLeftMultiply();
  void Handler__ObjectRightMultiply();
  void Handler__LocalVariablePrimitiveInplaceMultiply();
  void Handler__LocalVariableObjectInplaceMultiply();
  void Handler__LocalVariableObjectInplaceRightMultiply();
  void Handler__PrimitiveDivide();
  void Handler__ObjectDivide();
  void Handler__ObjectLeftDivide();
  void Handler__ObjectRightDivide();
  void Handler__LocalVariablePrimitiveInplaceDivide();
  void Handler__LocalVariableObjectInplaceDivide();
  void Handler__LocalVariableObjectInplaceRightDivide();
  void Handler__PrimitiveModulo();
  void Handler__LocalVariablePrimitiveInplaceModulo();
  void Handler__InitialiseArray();
  void Handler__ContractVariableDeclareAssign();
  void Handler__InvokeContractFunction();
  void Handler__PushLargeConstant();
  void Handler__PushMemberVariable();
  void Handler__PopToMemberVariable();
  void Handler__MemberVariablePrefixInc();
  void Handler__MemberVariablePrefixDec();
  void Handler__MemberVariablePostfixInc();
  void Handler__MemberVariablePostfixDec();
  void Handler__MemberVariablePrimitiveInplaceAdd();
  void Handler__MemberVariableObjectInplaceAdd();
  void Handler__MemberVariableObjectInplaceRightAdd();
  void Handler__MemberVariablePrimitiveInplaceSubtract();
  void Handler__MemberVariableObjectInplaceSubtract();
  void Handler__MemberVariableObjectInplaceRightSubtract();
  void Handler__MemberVariablePrimitiveInplaceMultiply();
  void Handler__MemberVariableObjectInplaceMultiply();
  void Handler__MemberVariableObjectInplaceRightMultiply();
  void Handler__MemberVariablePrimitiveInplaceDivide();
  void Handler__MemberVariableObjectInplaceDivide();
  void Handler__MemberVariableObjectInplaceRightDivide();
  void Handler__MemberVariablePrimitiveInplaceModulo();
  void Handler__PushSelf();
  void Handler__InvokeUserDefinedConstructor();
  void Handler__InvokeUserDefinedMemberFunction();

  friend class Object;
  friend class Module;
  friend class Generator;
};

template <typename T>
IfIsExternal<T, bool> ParameterPack::AddSingle(T val)
{
  if (vm_ == nullptr)
  {
    throw std::runtime_error("Cannot copy construct C++-to-Etch objects without a VM instance.");
  }
  using DecayedType = typename std::decay<T>::type;

  if (!vm_->HasCPPCopyConstructor<DecayedType>())
  {
    throw std::runtime_error("No C++-to-Etch copy constructor availble for type.");
  }

  AddInternal(vm_->CPPCopyConstruct<DecayedType>(val));
  return true;
}

}  // namespace vm
}  // namespace fetch
