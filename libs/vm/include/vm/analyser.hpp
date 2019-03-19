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

#include "vm/node.hpp"

namespace fetch {
namespace vm {

class Analyser
{
public:
  Analyser() = default;

  ~Analyser()
  {
    UnInitialise();
  }

  void Initialise();

  void UnInitialise();

  void CreateClassType(std::string const &name, TypeId id);

  void CreateTemplateInstantiationType(TypeId id, TypeId template_type_id,
                                       TypeIdArray const &parameter_type_ids);

  void CreateOpcodeFreeFunction(std::string const &name, Opcode opcode,
                                TypeIdArray const &parameter_type_ids, TypeId return_type_id);

  void CreateOpcodeTypeConstructor(TypeId type_id, Opcode opcode,
                                   TypeIdArray const &parameter_type_ids);

  void CreateOpcodeTypeFunction(TypeId type_id, std::string const &name, Opcode opcode,
                                TypeIdArray const &parameter_type_ids, TypeId return_type_id);

  void CreateOpcodeInstanceFunction(TypeId type_id, std::string const &name, Opcode opcode,
                                    TypeIdArray const &parameter_type_ids, TypeId return_type_id);

  void EnableOperator(TypeId type_id, Operator op);

  void EnableIndexOperator(TypeId type_id, TypeIdArray const &input_type_ids,
                           TypeId const &output_type_id);

  bool Analyse(BlockNodePtr const &root, TypeInfoTable &type_info_table, Strings &errors);

private:
  static std::string CONSTRUCTOR;

  using TypeTable     = std::unordered_map<TypeId, TypePtr>;
  using OperatorTable = std::unordered_map<Operator, Node::Kind>;
  using OpArray       = std::vector<Node::Kind>;
  using OpTable       = std::unordered_map<TypePtr, OpArray>;

  TypePtrArray      types_;
  TypeTable         type_table_;
  TypeInfoTable     type_info_table_;
  SymbolTablePtr    global_symbol_table_;
  TypeId            next_instantiation_type_id_;
  OperatorTable     operator_table_;
  TypePtr           any_type_;
  TypePtr           template_parameter1_type_;
  TypePtr           template_parameter2_type_;
  TypePtr           void_type_;
  TypePtr           null_type_;
  TypePtr           bool_type_;
  TypePtr           int8_type_;
  TypePtr           byte_type_;
  TypePtr           int16_type_;
  TypePtr           uint16_type_;
  TypePtr           int32_type_;
  TypePtr           uint32_type_;
  TypePtr           int64_type_;
  TypePtr           uint64_type_;
  TypePtr           float32_type_;
  TypePtr           float64_type_;
  TypePtr           integer_variant_type_;
  TypePtr           real_variant_type_;
  TypePtr           number_variant_type_;
  TypePtr           cast_variant_type_;
  TypePtr           matrix_type_;
  TypePtr           array_type_;
  TypePtr           map_type_;
  TypePtr           state_type_;
  TypePtr           address_type_;
  TypePtr           string_type_;
  OpTable           op_table_;
  OpTable           left_op_table_;
  OpTable           right_op_table_;
  BlockNodePtr      root_;
  BlockNodePtrArray blocks_;
  BlockNodePtrArray loops_;
  FunctionPtr       function_;
  Strings           errors_;

  void        AddError(Token const &token, std::string const &message);
  void        BuildBlock(BlockNodePtr const &block_node);
  void        BuildFunctionDefinition(BlockNodePtr const &parent_block_node,
                                      BlockNodePtr const &function_definition_node);
  void        BuildWhileStatement(BlockNodePtr const &while_statement_node);
  void        BuildForStatement(BlockNodePtr const &for_statement_node);
  void        BuildIfStatement(NodePtr const &if_statement_node);
  void        AnnotateBlock(BlockNodePtr const &block_node);
  void        AnnotateFunctionDefinitionStatement(BlockNodePtr const &function_definition_node);
  void        AnnotateWhileStatement(BlockNodePtr const &while_statement_node);
  void        AnnotateForStatement(BlockNodePtr const &for_statement_node);
  void        AnnotateIfStatement(NodePtr const &if_statement_node);
  void        AnnotateVarStatement(BlockNodePtr const &parent_block_node,
                                   NodePtr const &     var_statement_node);
  void        AnnotateReturnStatement(NodePtr const &return_statement_node);
  void        AnnotateConditionalBlock(BlockNodePtr const &conditional_node);
  bool        AnnotateTypeExpression(ExpressionNodePtr const &node);
  bool        AnnotateAssignOp(ExpressionNodePtr const &node);
  bool        AnnotateModuloAssignOp(ExpressionNodePtr const &node);
  bool        AnnotateArithmeticAssignOp(ExpressionNodePtr const &node);
  bool        AnnotateExpression(ExpressionNodePtr const &node);
  bool        AnnotateEqualityOp(ExpressionNodePtr const &node);
  bool        AnnotateRelationalOp(ExpressionNodePtr const &node);
  bool        AnnotateBinaryLogicalOp(ExpressionNodePtr const &node);
  bool        AnnotateUnaryLogicalOp(ExpressionNodePtr const &node);
  bool        AnnotateIncDecOp(ExpressionNodePtr const &node);
  bool        AnnotateUnaryMinusOp(ExpressionNodePtr const &node);
  bool        AnnotateModuloOp(ExpressionNodePtr const &node);
  bool        AnnotateArithmeticOp(ExpressionNodePtr const &node);
  bool        AnnotateIndexOp(ExpressionNodePtr const &node);
  bool        AnnotateDotOp(ExpressionNodePtr const &node);
  bool        AnnotateInvokeOp(ExpressionNodePtr const &node);
  bool        TestBlock(BlockNodePtr const &block_node);
  bool        IsWriteable(ExpressionNodePtr const &lhs);
  TypePtr     IsCompatible(ExpressionNodePtr const &node, ExpressionNodePtr const &lhs,
                           ExpressionNodePtr const &rhs);
  TypePtr     ConvertType(TypePtr const &type, TypePtr const &instantiated_template_type);
  bool        MatchType(TypePtr const &supplied_type, TypePtr const &expected_type) const;
  bool        MatchTypes(TypePtr const &type, TypePtrArray const &supplied_types,
                         TypePtrArray const &expected_types, TypePtrArray &actual_types);
  FunctionPtr FindFunction(TypePtr const &type, FunctionGroupPtr const &fg,
                           TypePtrArray const &supplied_types, TypePtrArray &actual_types);
  TypePtr     FindType(ExpressionNodePtr const &node);
  SymbolPtr   FindSymbol(ExpressionNodePtr const &node);
  SymbolPtr   SearchSymbolTables(std::string const &name);
  void        SetVariable(ExpressionNodePtr const &node, VariablePtr const &variable);
  void        SetLV(ExpressionNodePtr const &node, TypePtr const &type);
  void        SetRV(ExpressionNodePtr const &node, TypePtr const &type);
  void        SetType(ExpressionNodePtr const &node, TypePtr const &type);
  void        SetFunction(ExpressionNodePtr const &node, FunctionGroupPtr const &fg,
                          TypePtr const &fg_owner, bool function_invoked_on_instance);
  void        CreateMetaType(std::string const &name, TypeId id, TypePtr &type);
  void CreatePrimitiveType(std::string const &name, TypeId id, bool add_to_global_symbol_table,
                           TypePtr &type);
  void CreateClassType(std::string const &name, TypeId id, TypePtr &type);
  void CreateTemplateType(std::string const &name, TypeId id, TypePtrArray const &allowed_types,
                          TypePtr &type);
  void CreateTemplateInstantiationType(TypeId id, TypePtr const &template_type,
                                       TypePtrArray const &parameter_types, TypePtr &type);
  void CreateVariantType(std::string const &name, TypeId id, TypePtrArray const &allowed_types,
                         TypePtr &type);
  void InternalCreateTemplateInstantiationType(TypeId id, TypePtr const &template_type,
                                               TypePtrArray const &parameter_types,
                                               TypeIdArray const & parameter_type_ids,
                                               TypePtr &           type);
  void CreateOpcodeFreeFunction(std::string const &name, Opcode opcode,
                                TypePtrArray const &parameter_types, TypePtr const &return_type);
  void CreateOpcodeTypeConstructor(TypePtr const &type, Opcode opcode,
                                   TypePtrArray const &parameter_types);
  void CreateOpcodeTypeFunction(TypePtr const &type, std::string const &name, Opcode opcode,
                                TypePtrArray const &parameter_types, TypePtr const &return_type);
  void CreateOpcodeInstanceFunction(TypePtr const &type, std::string const &name, Opcode opcode,
                                    TypePtrArray const &parameter_types,
                                    TypePtr const &     return_type);
  FunctionPtr CreateUserFunction(std::string const &name, TypePtrArray const &parameter_types,
                                 VariablePtrArray const &parameter_variables,
                                 TypePtr const &         return_type);
  FunctionPtr CreateOpcodeFunction(std::string const &name, Function::Kind kind, Opcode opcode,
                                   TypePtrArray const &parameter_types, TypePtr const &return_type);

  void AddFunction(SymbolTablePtr const &symbol_table, FunctionPtr const &function);

  bool IsIntegerType(TypePtr const &type) const
  {
    return (type->id == TypeIds::Int8) || (type->id == TypeIds::Byte) ||
           (type->id == TypeIds::Int16) || (type->id == TypeIds::UInt16) ||
           (type->id == TypeIds::Int32) || (type->id == TypeIds::UInt32) ||
           (type->id == TypeIds::Int64) || (type->id == TypeIds::UInt64);
  }

  bool IsRealType(TypePtr const &type) const
  {
    return (type->id == TypeIds::Float32) || (type->id == TypeIds::Float64);
  }

  bool IsNumberType(TypePtr const &type) const
  {
    return IsIntegerType(type) || IsRealType(type);
  }

  void AddType(TypeId type_id, TypePtr const &type, TypeInfo const &type_info)
  {
    types_.push_back(type);
    type_table_.insert(std::pair<TypeId, TypePtr>(type_id, type));
    type_info_table_.insert(std::pair<TypeId, TypeInfo>(type_id, type_info));
  }

  TypePtr GetTypePtr(TypeId type_id)
  {
    auto it = type_table_.find(type_id);
    if (it != type_table_.end())
    {
      return it->second;
    }
    throw std::runtime_error("type id is not valid");
  }

  TypePtrArray GetTypePtrs(TypeIdArray const &type_ids)
  {
    TypePtrArray array;
    for (TypeId type_id : type_ids)
    {
      array.push_back(GetTypePtr(type_id));
    }
    return array;
  }

  void EnableIndexOperator(TypePtr const &type, TypePtrArray const &input_types,
                           TypePtr const &output_type)
  {
    type->index_input_types = input_types;
    type->index_output_type = output_type;
  }

  void EnableOperator(TypePtr const &type, Operator op)
  {
    EnableOp(type, operator_table_[op], op_table_);
  }

  void EnableLeftOperator(TypePtr const &type, Operator op)
  {
    EnableOp(type, operator_table_[op], left_op_table_);
  }

  void EnableRightOperator(TypePtr const &type, Operator op)
  {
    EnableOp(type, operator_table_[op], right_op_table_);
  }

  bool IsOpEnabled(TypePtr const &type, Node::Kind op) const
  {
    return IsOpEnabled(type, op, op_table_);
  }

  bool IsLeftOpEnabled(TypePtr const &type, Node::Kind op) const
  {
    return IsOpEnabled(type, op, left_op_table_);
  }

  bool IsRightOpEnabled(TypePtr const &type, Node::Kind op) const
  {
    return IsOpEnabled(type, op, right_op_table_);
  }

  void EnableOp(TypePtr const &type, Node::Kind op, OpTable &table)
  {
    auto it = table.find(type);
    if (it == table.end())
    {
      OpArray array;
      array.push_back(op);
      table.insert(std::pair<TypePtr, OpArray>(type, array));
    }
    else
    {
      OpArray &array = it->second;
      array.push_back(op);
    }
  }

  bool IsOpEnabled(TypePtr const &type, Node::Kind op, OpTable const &table) const
  {
    bool const is_instantiation = type->category == TypeCategory::TemplateInstantiation;
    TypePtr    t                = is_instantiation ? type->template_type : type;
    auto       it               = table.find(t);
    if (it != table.end())
    {
      auto const &array = it->second;
      for (auto const &element : array)
      {
        if (element == op)
        {
          // The type does support the op
          return true;
        }
      }
    }
    // The type does not support the op
    return false;
  }
};

}  // namespace vm
}  // namespace fetch
