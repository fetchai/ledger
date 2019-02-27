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

#include "vm/analyser.hpp"
#include "vm/generator.hpp"
#include "vm/parser.hpp"

namespace fetch {
namespace vm {

// Forward declarations
class Module;

class Compiler
{
public:
  Compiler(Module *module);
  ~Compiler();
  bool Compile(std::string const &source, std::string const &name, Script &script, Strings &errors);

private:
  void CreateClassType(std::string const &name, TypeId id)
  {
    analyser_.CreateClassType(name, id);
  }

  void CreateTemplateInstantiationType(TypeId id, TypeId template_type_id,
                                       TypeIdArray const &parameter_type_ids)
  {
    analyser_.CreateTemplateInstantiationType(id, template_type_id, parameter_type_ids);
  }

  void CreateOpcodeFreeFunction(std::string const &name, Opcode opcode,
                                TypeIdArray const &parameter_type_ids, TypeId return_type_id)
  {
    analyser_.CreateOpcodeFreeFunction(name, opcode, parameter_type_ids, return_type_id);
  }

  void CreateOpcodeTypeConstructor(TypeId type_id, Opcode opcode,
                                   TypeIdArray const &parameter_type_ids)
  {
    analyser_.CreateOpcodeTypeConstructor(type_id, opcode, parameter_type_ids);
  }

  void CreateOpcodeTypeFunction(TypeId type_id, std::string const &name, Opcode opcode,
                                TypeIdArray const &parameter_type_ids, TypeId return_type_id)
  {
    analyser_.CreateOpcodeTypeFunction(type_id, name, opcode, parameter_type_ids, return_type_id);
  }

  void CreateOpcodeInstanceFunction(TypeId type_id, std::string const &name, Opcode opcode,
                                    TypeIdArray const &parameter_type_ids, TypeId return_type_id)
  {
    analyser_.CreateOpcodeInstanceFunction(type_id, name, opcode, parameter_type_ids,
                                           return_type_id);
  }

  void EnableOperator(TypeId type_id, Operator op)
  {
    analyser_.EnableOperator(type_id, op);
  }

  void EnableIndexOperator(TypeId type_id, TypeIdArray const &input_type_ids,
                           TypeId const &output_type_id)
  {
    analyser_.EnableIndexOperator(type_id, input_type_ids, output_type_id);
  }

  Parser    parser_;
  Analyser  analyser_;
  Generator generator_;

  friend class Module;
};

}  // namespace vm
}  // namespace fetch
