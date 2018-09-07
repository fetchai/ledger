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

#include "vm/node.hpp"

#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace fetch {
namespace vm {
class Module;

class Analyser
{
public:
  Analyser(Module *module = nullptr);
  ~Analyser() {}
  bool Analyse(const BlockNodePtr &root, std::vector<std::string> &errors);

  template <typename T>
  friend class ClassInterface;
  friend class Module;
  friend class BaseModule;

private:
  template <typename T>
  void RegisterType(TypePtr const &ptr)
  {
    if (type_register_.find(std::type_index(typeid(T))) != type_register_.end())
    {
      throw std::runtime_error("Type already registered.");
    }
    type_register_[std::type_index(typeid(T))] = ptr;
  }

  template <typename T>
  TypePtr GetType()
  {
    if (type_register_.find(std::type_index(typeid(T))) == type_register_.end())
    {
      throw std::runtime_error("Could not find type.");
    }
    return type_register_[std::type_index(typeid(T))];
  }

  TypePtr GetType(std::type_index const &idx)
  {
    if (type_register_.find(idx) == type_register_.end())
    {
      throw std::runtime_error("Could not find type.");
    }
    return type_register_[idx];
  }

  std::unordered_map<std::type_index, TypePtr> type_register_;

  void    AddError(const Token &token, const std::string &message);
  void    BuildBlock(const BlockNodePtr &block_node);
  void    BuildFunctionDefinition(const BlockNodePtr &parent_block_node,
                                  const BlockNodePtr &function_definition_node);
  void    BuildWhileStatement(const BlockNodePtr &parent_block_node,
                              const BlockNodePtr &while_statement_node);
  void    BuildForStatement(const BlockNodePtr &parent_block_node,
                            const BlockNodePtr &for_statement_node);
  void    BuildIfStatement(const BlockNodePtr &parent_block_node, const NodePtr &if_statement_node);
  void    AnnotateBlock(const BlockNodePtr &block_node);
  void    AnnotateFunctionDefinitionStatement(const BlockNodePtr &function_definition_node);
  bool    TestBlock(const BlockNodePtr &block_node);
  void    AnnotateWhileStatement(const BlockNodePtr &while_statement_node);
  void    AnnotateForStatement(const BlockNodePtr &for_statement_node);
  void    AnnotateIfStatement(const NodePtr &if_statement_node);
  void    AnnotateVarStatement(const BlockNodePtr &parent_block_node,
                               const NodePtr &     var_statement_node);
  void    AnnotateReturnStatement(const NodePtr &return_statement_node);
  void    AnnotateConditionalBlock(const BlockNodePtr &conditional_node);
  bool    AnnotateTypeExpression(const ExpressionNodePtr &node);
  bool    AnnotateAssignOp(const ExpressionNodePtr &node);
  bool    AnnotateAddSubtractAssignOp(const ExpressionNodePtr &node);
  bool    AnnotateMultiplyAssignOp(const ExpressionNodePtr &node);
  bool    AnnotateDivideAssignOp(const ExpressionNodePtr &node);
  bool    IsWriteable(const ExpressionNodePtr &lhs);
  TypePtr IsAddSubtractCompatible(const ExpressionNodePtr &node, const ExpressionNodePtr &lhs,
                                  const ExpressionNodePtr &rhs);
  TypePtr IsMultiplyCompatible(const ExpressionNodePtr &node, const ExpressionNodePtr &lhs,
                               const ExpressionNodePtr &rhs);
  TypePtr IsDivideCompatible(const ExpressionNodePtr &node, const ExpressionNodePtr &lhs,
                             const ExpressionNodePtr &rhs);
  bool    AnnotateExpression(const ExpressionNodePtr &node);
  bool    AnnotateAddOp(const ExpressionNodePtr &node);
  bool    AnnotateSubtractOp(const ExpressionNodePtr &node);
  bool    AnnotateMultiplyOp(const ExpressionNodePtr &node);
  bool    AnnotateDivideOp(const ExpressionNodePtr &node);
  bool    AnnotateUnaryMinusOp(const ExpressionNodePtr &node);
  bool    AnnotateEqualityOp(const ExpressionNodePtr &node);
  bool    AnnotateRelationalOp(const ExpressionNodePtr &node);
  bool    AnnotateBinaryLogicalOp(const ExpressionNodePtr &node);
  bool    AnnotateUnaryLogicalOp(const ExpressionNodePtr &node);
  bool    AnnotateIncDecOp(const ExpressionNodePtr &node);
  bool    AnnotateIndexOp(const ExpressionNodePtr &node);
  bool    AnnotateDotOp(const ExpressionNodePtr &node);
  bool    AnnotateInvokeOp(const ExpressionNodePtr &node);

  FunctionPtr FindMatchingFunction(const FunctionGroupPtr &fg, const TypePtr &type,
                                   const std::vector<TypePtr> &input_types,
                                   std::vector<TypePtr> &      output_types);
  TypePtr     ConvertType(TypePtr type, TypePtr instantiated_template_type);
  TypePtr     FindType(const ExpressionNodePtr &node);
  SymbolPtr   FindSymbol(const ExpressionNodePtr &node);
  SymbolPtr   SearchSymbolTables(const std::string &name);
  void        SetVariable(const ExpressionNodePtr &node, const VariablePtr &variable);
  void        SetLV(const ExpressionNodePtr &node, const TypePtr &type);
  void        SetRV(const ExpressionNodePtr &node, const TypePtr &type);
  void        SetType(const ExpressionNodePtr &node, const TypePtr &type);
  void        SetFunction(const ExpressionNodePtr &node, const FunctionGroupPtr &fg,
                          const TypePtr &fg_owner, const bool function_invoked_on_instance);
  TypePtr     CreatePrimitiveType(const std::string &name, const TypeId id);
  TypePtr     CreateTemplateType(const std::string &name, const TypeId id);
  TypePtr     CreateTemplateInstantiationType(const std::string &name, const TypeId id,
                                              const TypePtr &             template_type,
                                              const std::vector<TypePtr> &template_parameter_types);
  TypePtr     CreateClassType(const std::string &name, const TypeId id);
  FunctionPtr CreateUserFunction(const std::string &             name,
                                 const std::vector<TypePtr> &    parameter_types,
                                 const std::vector<VariablePtr> &parameter_variables,
                                 const TypePtr &                 return_type);
  FunctionPtr CreateOpcodeFunction(const std::string &name, const Function::Kind kind,
                                   const std::vector<TypePtr> &parameter_types,
                                   const TypePtr &return_type, const Opcode opcode);
  void        AddFunction(const SymbolTablePtr &symbols, const FunctionPtr &function);
  void CreateOpcodeFunction(const std::string &name, const std::vector<TypePtr> &parameter_types,
                            const TypePtr &return_type, const Opcode opcode);
  void CreateOpcodeTypeFunction(const TypePtr &type, const std::string &name,
                                const std::vector<TypePtr> &parameter_types,
                                const TypePtr &return_type, const Opcode opcode);
  void CreateOpcodeInstanceFunction(const TypePtr &type, const std::string &name,
                                    const std::vector<TypePtr> &parameter_types,
                                    const TypePtr &return_type, const Opcode opcode);

  static std::string CONSTRUCTOR;

  SymbolTablePtr symbols_;

  TypePtr template_instantiation_type_;
  TypePtr template_parameter_type1_;
  TypePtr template_parameter_type2_;
  TypePtr numeric_type_;

  TypePtr matrix_template_type_;
  TypePtr array_template_type_;

  TypePtr void_type_;
  TypePtr null_type_;
  TypePtr bool_type_;
  TypePtr int8_type_;
  TypePtr byte_type_;
  TypePtr int16_type_;
  TypePtr uint16_type_;
  TypePtr int32_type_;
  TypePtr uint32_type_;
  TypePtr int64_type_;
  TypePtr uint64_type_;
  TypePtr float32_type_;
  TypePtr float64_type_;
  TypePtr string_type_;
  TypePtr matrix_float32_type_;
  TypePtr matrix_float64_type_;
  TypePtr array_bool_type_;
  TypePtr array_int8_type_;
  TypePtr array_byte_type_;
  TypePtr array_int16_type_;
  TypePtr array_uint16_type_;
  TypePtr array_int32_type_;
  TypePtr array_uint32_type_;
  TypePtr array_int64_type_;
  TypePtr array_uint64_type_;
  TypePtr array_float32_type_;
  TypePtr array_float64_type_;
  TypePtr array_string_type_;
  TypePtr array_matrix_float32_type_;
  TypePtr array_matrix_float64_type_;

  BlockNodePtr              root_;
  std::vector<BlockNodePtr> blocks_;
  std::vector<BlockNodePtr> loops_;
  FunctionPtr               function_;
  std::vector<std::string>  errors_;
};

}  // namespace vm
}  // namespace fetch
