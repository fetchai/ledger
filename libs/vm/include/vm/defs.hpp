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

#include "vm/opcodes.hpp"
#include "vm/typeids.hpp"
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace vm {

class VM;
struct Object
{
  Object()
  {}
  Object(TypeId const &type_id__, VM *vm__)
  {
    count   = 1;
    type_id = type_id__;
    vm      = vm__;
  }
  virtual ~Object()
  {}
  void AddRef()
  {
    ++count;
  }
  void     Release();
  uint32_t count;
  TypeId   type_id;
  VM *     vm;
};

union Variant
{
  int8_t   i8;
  uint8_t  ui8;
  int16_t  i16;
  uint16_t ui16;
  int32_t  i32;
  uint32_t ui32;
  int64_t  i64;
  uint64_t ui64;
  float    f32;
  double   f64;
  Object * object;

  void Zero()
  {
    ui64 = 0;
  }

  void Get(int8_t &value)
  {
    value = i8;
  }
  void Get(uint8_t &value)
  {
    value = ui8;
  }
  void Get(int16_t &value)
  {
    value = i16;
  }
  void Get(uint16_t &value)
  {
    value = ui16;
  }
  void Get(int32_t &value)
  {
    value = i32;
  }
  void Get(uint32_t &value)
  {
    value = ui32;
  }
  void Get(int64_t &value)
  {
    value = i64;
  }
  void Get(uint64_t &value)
  {
    value = ui64;
  }
  void Get(float &value)
  {
    value = f32;
  }
  void Get(double &value)
  {
    value = f64;
  }
  void Get(Object *&value)
  {
    value = object;
  }

  void Set(int8_t value)
  {
    i8 = value;
  }
  void Set(uint8_t value)
  {
    ui8 = value;
  }
  void Set(int16_t value)
  {
    i16 = value;
  }
  void Set(uint16_t value)
  {
    ui16 = value;
  }
  void Set(int32_t value)
  {
    i32 = value;
  }
  void Set(uint32_t value)
  {
    ui32 = value;
  }
  void Set(int64_t value)
  {
    i64 = value;
  }
  void Set(uint64_t value)
  {
    ui64 = value;
  }
  void Set(float value)
  {
    f32 = value;
  }
  void Set(double value)
  {
    f64 = value;
  }
  void Set(Object *value)
  {
    object = value;
  }
};

struct Value
{
  Value()
  {
    type_id = TypeId::Unknown;
    variant.Zero();
  }

  ~Value()
  {
    if (IsObject())
    {
      Release();
    }
  }

  Value(Value const &other)
  {
    type_id = other.type_id;
    variant = other.variant;
    if (IsObject())
    {
      AddRef();
    }
  }

  Value &operator=(Value const &other)
  {
    const bool is_object       = IsObject();
    const bool other_is_object = other.IsObject();
    if (is_object && other_is_object)
    {
      if (variant.object != other.variant.object)
      {
        Release();
        variant.object = other.variant.object;
        AddRef();
      }
      type_id = other.type_id;
      return *this;
    }
    if (is_object)
    {
      Release();
    }
    type_id = other.type_id;
    variant = other.variant;
    if (other_is_object)
    {
      AddRef();
    }
    return *this;
  }

  Value(Value &&other)
  {
    type_id       = other.type_id;
    variant       = other.variant;
    other.type_id = TypeId::Unknown;
    other.variant.Zero();
  }

  Value &operator=(Value &&other)
  {
    const bool is_object       = IsObject();
    const bool other_is_object = other.IsObject();
    if (is_object && other_is_object)
    {
      if (variant.object != other.variant.object)
      {
        Release();
        type_id       = other.type_id;
        variant       = other.variant;
        other.type_id = TypeId::Unknown;
        other.variant.Zero();
      }
      return *this;
    }
    if (is_object)
    {
      Release();
    }
    type_id       = other.type_id;
    variant       = other.variant;
    other.type_id = TypeId::Unknown;
    other.variant.Zero();
    return *this;
  }

  void Copy(Value const &other)
  {
    type_id = other.type_id;
    variant = other.variant;
    if (IsObject())
    {
      AddRef();
    }
  }

  void Reset()
  {
    if (IsObject())
    {
      Release();
    }
    type_id = TypeId::Unknown;
    variant.Zero();
  }

  void PrimitiveReset()
  {
    type_id = TypeId::Unknown;
    variant.Zero();
  }

  void AddRef()
  {
    if (variant.object)
    {
      variant.object->AddRef();
    }
  }

  void Release()
  {
    if (variant.object)
    {
      variant.object->Release();
    }
  }

  template <typename T>
  void SetPrimitive(T const &primitive, TypeId const &type_id__)
  {
    if (IsObject())
    {
      Release();
    }
    type_id = type_id__;
    variant.Set(primitive);
  }

  void SetObject(Object *object, TypeId const &type_id__)
  {
    if (IsObject())
    {
      Release();
    }
    type_id        = type_id__;
    variant.object = object;
  }

  bool IsObject() const
  {
    return type_id > TypeId::PrimitivesObjectsDivider;
  }
  TypeId  type_id;
  Variant variant;
};

struct Script
{
  Script()
  {}
  Script(const std::string &name__)
  {
    name = name__;
  }
  struct Instruction
  {
    Instruction(Opcode const &opcode__, uint16_t const &line__)
    {
      opcode  = opcode__;
      line    = line__;
      index   = 0;
      type_id = TypeId::Unknown;
      variant.Zero();
    }
    Opcode   opcode;
    uint16_t line;
    Index    index;  // index of variable, or index into instructions (pc)
    TypeId   type_id;
    Variant  variant;
  };

  using Instructions = std::vector<Instruction>;

  struct Variable
  {
    Variable(std::string const &name__, TypeId const &type_id__)
    {
      name    = name__;
      type_id = type_id__;
    }
    std::string name;
    TypeId      type_id;
  };

  struct Function
  {
    Function(std::string const &name__, int const &num_parameters__)
    {
      name           = name__;
      num_variables  = 0;
      num_parameters = num_parameters__;
    }
    Index AddVariable(std::string const &name, TypeId const &type_id)
    {
      const Index index = (Index)num_variables++;
      variables.push_back(Variable(name, type_id));
      return index;
    }
    Index AddInstruction(Instruction &instruction)
    {
      const Index pc = (Index)instructions.size();
      instructions.push_back(std::move(instruction));
      return pc;
    }
    std::string           name;
    int                   num_variables;  // parameters + locals
    int                   num_parameters;
    std::vector<Variable> variables;  // parameters + locals
    Instructions          instructions;
  };

  using Functions = std::vector<Function>;
  std::string                            name;
  std::vector<std::string>               strings;
  Functions                              functions;
  std::unordered_map<std::string, Index> map;

  Index AddFunction(Function &function)
  {
    const Index index  = (Index)functions.size();
    map[function.name] = index;
    functions.push_back(std::move(function));
    return index;
  }

  Function const *FindFunction(std::string const &name) const
  {
    auto it = map.find(name);
    if (it != map.end())
    {
      return &(functions[it->second]);
    }
    return nullptr;
  }
};

}  // namespace vm
}  // namespace fetch
