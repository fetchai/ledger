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
#include "vm/ir_builder.hpp"
#include "vm/parser.hpp"

namespace fetch {
namespace vm {

class Module;

class Compiler
{
public:
  Compiler(Module *module);
  ~Compiler();
  bool Compile(std::string const &filename, std::string const &source,
      std::string const &name, IR &ir, std::vector<std::string> &errors);

private:
  void CreateClassType(std::string const &name, TypeIndex type_index)
  {
    analyser_.CreateClassType(name, type_index);
  }

  void CreateInstantiationType(TypeIndex type_index, TypeIndex template_type_index,
                               TypeIndexArray const &parameter_type_index_array)
  {
    analyser_.CreateInstantiationType(type_index, template_type_index, parameter_type_index_array);
  }

  void CreateFreeFunction(std::string const &name,
                                TypeIndexArray const &parameter_type_index_array,
                                TypeIndex return_type_index,
                                Handler const &handler)
  {
    analyser_.CreateFreeFunction(name, parameter_type_index_array, return_type_index, handler);
  }

  void CreateConstructor(TypeIndex type_index,
                                   TypeIndexArray const &parameter_type_index_array,
                                   Handler const &handler)
  {
    analyser_.CreateConstructor(type_index, parameter_type_index_array, handler);
  }

  void CreateStaticMemberFunction(TypeIndex type_index,
                                std::string const &function_name,
                                TypeIndexArray const &parameter_type_index_array,
                                TypeIndex return_type_index,
                                Handler const &handler)
  {
    analyser_.CreateStaticMemberFunction(type_index, function_name, parameter_type_index_array,
        return_type_index, handler);
  }

  void CreateMemberFunction(TypeIndex type_index,
                                std::string const &function_name,
                                TypeIndexArray const &parameter_type_index_array,
                                TypeIndex return_type_index,
                                Handler const &handler)
  {
    analyser_.CreateMemberFunction(type_index, function_name,
                                        parameter_type_index_array,
                                           return_type_index, handler);
  }

  void EnableOperator(TypeIndex type_index, Operator op)
  {
    analyser_.EnableOperator(type_index, op);
  }

  void EnableIndexOperator(TypeIndex type_index,
      TypeIndexArray const &input_type_index_array,
                           TypeIndex output_type_index,
                           Handler const &get_handler, Handler const &set_handler)
  {
    analyser_.EnableIndexOperator(type_index, input_type_index_array, output_type_index,
        get_handler, set_handler);
  }

  void GetDetails(TypeInfoArray &type_info_array, TypeInfoMap &type_info_map,
      RegisteredTypes &registered_types, FunctionInfoArray &function_info_array)
  {
    analyser_.GetDetails(type_info_array, type_info_map, registered_types, function_info_array);
  }

  Parser    parser_;
  Analyser  analyser_;
  IRBuilder builder_;

  friend class Module;
};

}  // namespace vm
}  // namespace fetch
