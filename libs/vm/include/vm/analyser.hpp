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

#include "meta/type_util.hpp"
#include "vm/node.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

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

  void CreateClassType(std::string const &name, TypeIndex type_index);

  void CreateInstantiationType(TypeIndex type_index, TypeIndex template_type_index,
                               TypeIndexArray const &parameter_type_index_array);

  void CreateFreeFunction(std::string const &name, TypeIndexArray const &parameter_type_index_array,
                          TypeIndex return_type_index, Handler const &handler);

  void CreateConstructor(TypeIndex type_index, TypeIndexArray const &parameter_type_index_array,
                         Handler const &handler);

  void CreateStaticMemberFunction(TypeIndex type_index, std::string const &function_name,
                                  TypeIndexArray const &parameter_type_index_array,
                                  TypeIndex return_type_index, Handler const &handler);

  void CreateMemberFunction(TypeIndex type_index, std::string const &function_name,
                            TypeIndexArray const &parameter_type_index_array,
                            TypeIndex return_type_index, Handler const &handler);

  void EnableOperator(TypeIndex type_index, Operator op);

  void EnableIndexOperator(TypeIndex type_index, TypeIndexArray const &input_type_index_array,
                           TypeIndex output_type_index, Handler const &get_handler,
                           Handler const &set_handler);

  bool Analyse(BlockNodePtr const &root, std::vector<std::string> &errors);

  void GetDetails(TypeInfoArray &type_info_array, TypeInfoMap &type_info_map,
                  RegisteredTypes &registered_types, FunctionInfoArray &function_info_array)
  {
    type_info_array     = type_info_array_;
    type_info_map       = type_info_map_;
    registered_types    = registered_types_;
    function_info_array = function_info_array_;
  }

private:
  static std::string CONSTRUCTOR;
  static std::string GET_INDEXED_VALUE;
  static std::string SET_INDEXED_VALUE;
  using OperatorMap = std::unordered_map<NodeKind, Operator>;

  struct TypeMap
  {
    void Add(TypeIndex type_index, TypePtr const &type)
    {
      map[type_index] = type;
    }
    TypePtr Find(TypeIndex type_index)
    {
      auto it = map.find(type_index);
      if (it != map.end())
      {
        return it->second;
      }
      return nullptr;
    }
    void Reset()
    {
      for (auto &it : map)
      {
        it.second->Reset();
      }
    }
    std::unordered_map<TypeIndex, TypePtr> map;
  };

  struct FunctionSet
  {
    void Add(std::string const &unique_id)
    {
      set.insert(unique_id);
    }
    bool Find(std::string const &unique_id)
    {
      auto it = set.find(unique_id);
      return it != set.end();
    }
    std::unordered_set<std::string> set;
  };

  OperatorMap       operator_map_;
  TypeMap           type_map_;
  TypeInfoArray     type_info_array_;
  TypeInfoMap       type_info_map_;
  RegisteredTypes   registered_types_;
  FunctionInfoArray function_info_array_;
  FunctionSet       function_set_;

  SymbolTablePtr symbols_;
  TypePtr        null_type_;
  TypePtr        void_type_;
  TypePtr        bool_type_;
  TypePtr        int8_type_;
  TypePtr        uint8_type_;
  TypePtr        int16_type_;
  TypePtr        uint16_type_;
  TypePtr        int32_type_;
  TypePtr        uint32_type_;
  TypePtr        int64_type_;
  TypePtr        uint64_type_;
  TypePtr        float32_type_;
  TypePtr        float64_type_;
  TypePtr        fixed32_type_;
  TypePtr        fixed64_type_;
  TypePtr        string_type_;
  TypePtr        address_type_;
  TypePtr        template_parameter1_type_;
  TypePtr        template_parameter2_type_;
  TypePtr        any_type_;
  TypePtr        any_primitive_type_;
  TypePtr        any_integer_type_;
  TypePtr        any_floating_point_type_;
  TypePtr        matrix_type_;
  TypePtr        array_type_;
  TypePtr        map_type_;
  TypePtr        sharded_state_type_;
  TypePtr        state_type_;

  BlockNodePtr             root_;
  BlockNodePtrArray        blocks_;
  BlockNodePtrArray        loops_;
  FunctionPtr              function_;
  std::vector<std::string> errors_;

  void    AddError(uint16_t line, std::string const &message);
  void    BuildBlock(BlockNodePtr const &block_node);
  void    BuildFile(BlockNodePtr const &file_node);
  void    BuildFunctionDefinition(BlockNodePtr const &parent_block_node,
                                  BlockNodePtr const &function_definition_node);
  void    BuildWhileStatement(BlockNodePtr const &while_statement_node);
  void    BuildForStatement(BlockNodePtr const &for_statement_node);
  void    BuildIfStatement(NodePtr const &if_statement_node);
  void    AnnotateBlock(BlockNodePtr const &block_node);
  void    AnnotateFile(BlockNodePtr const &file_node);
  void    AnnotateFunctionDefinitionStatement(BlockNodePtr const &function_definition_node);
  void    AnnotateWhileStatement(BlockNodePtr const &while_statement_node);
  void    AnnotateForStatement(BlockNodePtr const &for_statement_node);
  void    AnnotateIfStatement(NodePtr const &if_statement_node);
  void    AnnotateVarStatement(BlockNodePtr const &parent_block_node,
                               NodePtr const &     var_statement_node);
  void    AnnotateReturnStatement(NodePtr const &return_statement_node);
  void    AnnotateConditionalBlock(BlockNodePtr const &conditional_node);
  bool    AnnotateTypeExpression(ExpressionNodePtr const &node);
  bool    AnnotateAssignOp(ExpressionNodePtr const &node);
  bool    AnnotateInplaceArithmeticOp(ExpressionNodePtr const &node);
  bool    AnnotateInplaceModuloOp(ExpressionNodePtr const &node);
  bool    AnnotateLHSExpression(ExpressionNodePtr const &parent, ExpressionNodePtr const &lhs);
  bool    AnnotateExpression(ExpressionNodePtr const &node);
  bool    AnnotateEqualityOp(ExpressionNodePtr const &node);
  bool    AnnotateRelationalOp(ExpressionNodePtr const &node);
  bool    AnnotateBinaryLogicalOp(ExpressionNodePtr const &node);
  bool    AnnotateUnaryLogicalOp(ExpressionNodePtr const &node);
  bool    AnnotatePrefixPostfixOp(ExpressionNodePtr const &node);
  bool    AnnotateNegateOp(ExpressionNodePtr const &node);
  bool    AnnotateArithmeticOp(ExpressionNodePtr const &node);
  bool    AnnotateModuloOp(ExpressionNodePtr const &node);
  bool    AnnotateIndexOp(ExpressionNodePtr const &node);
  bool    AnnotateDotOp(ExpressionNodePtr const &node);
  bool    AnnotateInvokeOp(ExpressionNodePtr const &node);
  bool    AnnotateArrayExpr(ExpressionNodePtr const &node);
  bool    TestBlock(BlockNodePtr const &block_node);
  bool    IsWriteable(ExpressionNodePtr const &lhs);
  bool    AnnotateArithmetic(ExpressionNodePtr const &node, ExpressionNodePtr const &lhs,
                             ExpressionNodePtr const &rhs);
  TypePtr ConvertType(TypePtr const &type, TypePtr const &instantiated_template_type);

  bool        MatchType(TypePtr const &supplied_type, TypePtr const &expected_type) const;
  bool        MatchTypes(TypePtr const &type, TypePtrArray const &supplied_types,
                         TypePtrArray const &expected_types, TypePtrArray &actual_types);
  FunctionPtr FindFunction(TypePtr const &type, FunctionGroupPtr const &fg,
                           TypePtrArray const &supplied_types, TypePtrArray &actual_types);
  TypePtr     FindType(ExpressionNodePtr const &node);
  SymbolPtr   FindSymbol(ExpressionNodePtr const &node);
  SymbolPtr   SearchSymbols(std::string const &name);
  void        SetVariableExpression(ExpressionNodePtr const &node, VariablePtr const &variable);
  void        SetLVExpression(ExpressionNodePtr const &node, TypePtr const &type);
  void        SetRVExpression(ExpressionNodePtr const &node, TypePtr const &type);
  void        SetTypeExpression(ExpressionNodePtr const &node, TypePtr const &type);
  void        SetFunctionGroupExpression(ExpressionNodePtr const &node, FunctionGroupPtr const &fg,
                                         TypePtr const &fg_owner, bool function_invoked_on_instance);
  void        CreatePrimitiveType(std::string const &type_name, TypeIndex type_index,
                                  bool add_to_symbol_table, TypeId type_id, TypePtr &type);
  void        CreateMetaType(std::string const &type_name, TypeIndex type_index, TypeId type_id,
                             TypePtr &type);
  void        CreateClassType(std::string const &type_name, TypeIndex type_index, TypeId type_id,
                              TypePtr &type);
  void        CreateTemplateType(std::string const &type_name, TypeIndex type_index,
                                 TypePtrArray const &allowed_types, TypeId type_id, TypePtr &type);
  void        CreateInstantiationType(TypeIndex type_index, TypePtr const &template_type,
                                      TypePtrArray const &parameter_types, TypeId type_id, TypePtr &type);
  void        CreateGroupType(std::string const &type_name, TypeIndex type_index,
                              TypePtrArray const &allowed_types, TypeId type_id, TypePtr &type);
  TypePtr     InternalCreateInstantiationType(TypeKind type_kind, TypePtr const &template_type,
                                              TypePtrArray const &parameter_types);
  void        CreateFreeFunction(std::string const &name, TypePtrArray const &parameter_types,
                                 TypePtr const &return_type, Handler const &handler);
  void        CreateConstructor(TypePtr const &type, TypePtrArray const &parameter_types,
                                Handler const &handler);

  void        CreateStaticMemberFunction(TypePtr const &type, std::string const &name,
                                         TypePtrArray const &parameter_types, TypePtr const &return_type,
                                         Handler const &handler);
  void        CreateMemberFunction(TypePtr const &type, std::string const &name,
                                   TypePtrArray const &parameter_types, TypePtr const &return_type,
                                   Handler const &handler);
  FunctionPtr CreateUserDefinedFreeFunction(std::string const &     name,
                                            TypePtrArray const &    parameter_types,
                                            VariablePtrArray const &parameter_variables,
                                            TypePtr const &         return_type);
  void        EnableIndexOperator(TypePtr const &type, TypePtrArray const &input_types,
                                  TypePtr const &output_type, Handler const &get_handler,
                                  Handler const &set_handler);
  void        AddTypeInfo(TypeInfo const &info, TypeId type_id, TypePtr const &type);
  void        AddFunctionInfo(FunctionPtr const &function, Handler const &handler);

  std::string BuildUniqueId(TypePtr const &type, std::string const &function_name,
                            TypePtrArray const &parameter_types, TypePtr const &return_type);
  void        AddFunctionToSymbolTable(SymbolTablePtr const &symbols, FunctionPtr const &function);

  TypePtr GetType(TypeIndex type_index)
  {
    TypePtr type = type_map_.Find(type_index);
    if (type)
    {
      return type;
    }
    throw std::runtime_error("type index has not been registered for the following type:\n " +
                             std::string(type_index.name()));
  }

  TypePtrArray GetTypes(TypeIndexArray const &type_index_array)
  {
    TypePtrArray array;
    for (auto const &type_index : type_index_array)
    {
      array.push_back(GetType(type_index));
    }
    return array;
  }

  Operator GetOperator(NodeKind node_kind)
  {
    auto it = operator_map_.find(node_kind);
    if (it != operator_map_.end())
    {
      return it->second;
    }
    return Operator::Unknown;
  }

  static void EnableOperator(Operators &ops, Operator op)
  {
    ops.insert(op);
  }

  static bool IsOperatorEnabled(Operators const &ops, Operator op)
  {
    return ops.find(op) != ops.end();
  }

  static void EnableOperator(TypePtr const &type, Operator op)
  {
    EnableOperator(type->ops, op);
  }

  template <typename... Operators>
  static std::enable_if_t<type_util::ConjunctionV<std::is_same<Operator, Operators>...>>
  EnableOperator(TypePtr const &type, Operators... ops)
  {
    value_util::ForEach([&type](auto op) { EnableOperator(type, op); }, ops...);
  }

  static void EnableLeftOperator(TypePtr const &type, Operator op)
  {
    EnableOperator(type->left_ops, op);
  }

  template <typename... Operators>
  static std::enable_if_t<type_util::ConjunctionV<std::is_same<Operator, Operators>...>>
  EnableLeftOperator(TypePtr const &type, Operators... ops)
  {
    value_util::ForEach([&type](auto op) { EnableLeftOperator(type, op); }, ops...);
  }

  static void EnableRightOperator(TypePtr const &type, Operator op)
  {
    EnableOperator(type->right_ops, op);
  }

  template <typename... Operators>
  static std::enable_if_t<type_util::ConjunctionV<std::is_same<Operator, Operators>...>>
  EnableRightOperator(TypePtr const &type, Operators... ops)
  {
    value_util::ForEach([&type](auto op) { Analyser::EnableRightOperator(type, op); }, ops...);
  }

  static bool IsOperatorEnabled(TypePtr const &type, Operator op)
  {
    bool const is_instantiation = type->IsInstantiation();
    TypePtr    t                = is_instantiation ? type->template_type : type;
    return IsOperatorEnabled(t->ops, op);
  }

  static bool IsLeftOperatorEnabled(TypePtr const &type, Operator op)
  {
    bool const is_instantiation = type->IsInstantiation();
    TypePtr    t                = is_instantiation ? type->template_type : type;
    return IsOperatorEnabled(t->left_ops, op);
  }

  static bool IsRightOperatorEnabled(TypePtr const &type, Operator op)
  {
    bool const is_instantiation = type->IsInstantiation();
    TypePtr    t                = is_instantiation ? type->template_type : type;
    return IsOperatorEnabled(t->right_ops, op);
  }

  bool IsArray(TypePtr const &type) const
  {
    return type && type->IsInstantiation() && type->template_type == array_type_;
  }

  /**
   * Annotates empty array (if any) based on provided array type (if any).
   * If rhs is an empty array of yet-unknown type,
   * lhs_type should be an Array instantiation.
   * In that case, rhs's type is set to lhs_type.
   *
   * @param line
   * @param lhs_type
   * @param rhs
   * @param message Error text to be added when returning false.
   * @return false iff rhs is an empty array but lhs_type is not an array instantiation.
   */
  bool CloseEmptyArray(uint16_t line, TypePtr const &lhs_type, ExpressionNodePtr &rhs,
                       std::string const &message = "incompatible types")
  {
    if (IsEmptyArray(rhs))
    {
      if (!IsArray(lhs_type))
      {
        AddError(line, message);
        return false;
      }
      rhs->type = lhs_type;
    }
    return true;
  }

  template <class... TailParams>
  std::enable_if_t<type_util::ConjunctionV<std::is_same<TailParams, std::string>...>, TypePtr>
  InstantiateTemplate(std::uint16_t line, std::string template_name, std::string head_param,
                      TailParams... tail_params)
  {
    std::string type_name = value_util::Accumulate(
                                [](std::string type_name, auto const &param) {
                                  return std::move(type_name) + ", " + param;
                                },
                                template_name + '<' + head_param, tail_params...) +
                            '>';

    auto ret_val = CreateExpressionNode(NodeKind::Template, std::move(type_name), line);
    ret_val->children.push_back(
        CreateExpressionNode(NodeKind::Identifier, std::move(template_name), line));
    ret_val->children.push_back(
        CreateExpressionNode(NodeKind::Identifier, std::move(head_param), line));
    value_util::ForEach(
        [&](std::string param) {
          ret_val->children.push_back(
              CreateExpressionNode(NodeKind::Identifier, std::move(param), line));
        },
        std::move(tail_params)...);

    return FindType(ret_val);
  }
};

}  // namespace vm
}  // namespace fetch
