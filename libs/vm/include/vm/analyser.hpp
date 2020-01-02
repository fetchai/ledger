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

#include "vm/node.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
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
  Analyser()  = default;
  ~Analyser() = default;

  void Initialise();

  void UnInitialise();

  void CreateClassType(std::string const &name, TypeIndex type_index);

  void CreateTemplateType(std::string const &name, TypeIndex type_index,
                          TypeIndexArray const &allowed_types_index_array);

  void CreateTemplateInstantiationType(TypeIndex type_index, TypeIndex template_type_index,
                                       TypeIndexArray const &template_parameter_type_index_array);

  void CreateFreeFunction(std::string const &name, TypeIndexArray const &parameter_type_index_array,
                          TypeIndex return_type_index, Handler const &handler,
                          ChargeAmount static_charge);

  void CreateConstructor(TypeIndex type_index, TypeIndexArray const &parameter_type_index_array,
                         Handler const &handler, ChargeAmount static_charge);

  void CreateStaticMemberFunction(TypeIndex type_index, std::string const &function_name,
                                  TypeIndexArray const &parameter_type_index_array,
                                  TypeIndex return_type_index, Handler const &handler,
                                  ChargeAmount static_charge);

  void CreateMemberFunction(TypeIndex type_index, std::string const &function_name,
                            TypeIndexArray const &parameter_type_index_array,
                            TypeIndex return_type_index, Handler const &handler,
                            ChargeAmount static_charge);

  void EnableOperator(TypeIndex type_index, Operator op);

  void EnableLeftOperator(TypeIndex type_index, Operator op);

  void EnableRightOperator(TypeIndex type_index, Operator op);

  void EnableIndexOperator(TypeIndex type_index, TypeIndexArray const &input_type_index_array,
                           TypeIndex output_type_index, Handler const &get_handler,
                           Handler const &set_handler, ChargeAmount get_static_charge,
                           ChargeAmount set_static_charge);

  bool Analyse(BlockNodePtr const &root, std::vector<std::string> &errors);

  void GetDetails(TypeInfoArray &type_info_array, TypeInfoMap &type_info_map,
                  RegisteredTypes &registered_types, FunctionInfoArray &function_info_array) const
  {
    type_info_array     = type_info_array_;
    type_info_map       = type_info_map_;
    registered_types    = registered_types_;
    function_info_array = function_info_array_;
  }

private:
  static std::string const CONSTRUCTOR;
  static std::string const GET_INDEXED_VALUE;
  static std::string const SET_INDEXED_VALUE;

  static const uint16_t MAX_NESTED_BLOCKS                    = 256;
  static const uint16_t MAX_STATE_DEFINITIONS                = 256;
  static const uint16_t MAX_CONTRACT_DEFINITIONS             = 64;
  static const uint16_t MAX_FUNCTIONS_PER_CONTRACT           = 256;
  static const uint16_t MAX_USER_DEFINED_TYPES               = 256;
  static const uint16_t MAX_USER_DEFINED_INSTANTIATION_TYPES = 256;
  static const uint16_t MAX_FREE_FUNCTIONS                   = 256;
  static const uint16_t MAX_MEMBER_FUNCTIONS_PER_TYPE        = 256;
  static const uint16_t MAX_MEMBER_VARIABLES_PER_TYPE        = 256;
  static const uint16_t MAX_PARAMETERS_PER_FUNCTION          = 16;
  static const uint16_t MAX_LOCALS_PER_FUNCTION              = 256;

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

  struct StringSet
  {
    void Add(std::string const &s)
    {
      set.insert(s);
    }
    bool Find(std::string const &s)
    {
      auto it = set.find(s);
      return it != set.end();
    }
    std::unordered_set<std::string> set;
  };

  struct FunctionMap
  {
    void Add(FunctionPtr const &function)
    {
      map[function->unique_name] = function;
    }
    FunctionPtr Find(std::string const &unique_name)
    {
      auto it = map.find(unique_name);
      if (it != map.end())
      {
        return it->second;
      }
      return nullptr;
    }
    std::unordered_map<std::string, FunctionPtr> map;
  };

  struct NameToTypePtrMap
  {
    void Add(std::string const &name, TypePtr const &type)
    {
      map[name] = type;
    }
    TypePtr Find(std::string const &name)
    {
      auto it = map.find(name);
      if (it != map.end())
      {
        return it->second;
      }
      return nullptr;
    }
    void Clear()
    {
      map.clear();
    }
    std::unordered_map<std::string, TypePtr> map;
  };

  struct LedgerRestrictionMetadata
  {
    struct NodeAndFilename
    {
      NodeAndFilename(NodePtr node__, std::string filename__)
        : node{std::move(node__)}
        , filename{std::move(filename__)}
      {}

      NodePtr     node;
      std::string filename;
    };

    std::vector<NodeAndFilename> init_functions{};
    std::vector<NodeAndFilename> clear_functions{};
    std::vector<NodeAndFilename> objective_functions{};
    std::vector<NodeAndFilename> problem_functions{};
    std::vector<NodeAndFilename> work_functions{};
  };

  struct Error
  {
    bool operator<(Error const &other) const
    {
      return line < other.line;
    }
    uint16_t    line{};
    std::string message;
  };

  struct FileErrors
  {
    explicit FileErrors(std::string filename__)
      : filename{std::move(filename__)}
    {}
    std::string        filename;
    std::vector<Error> errors;
  };
  using FileErrorsArray = std::vector<FileErrors>;

  std::vector<std::string> GetErrorList()
  {
    std::vector<std::string> list;
    for (auto &it : file_errors_array_)
    {
      std::stable_sort(it.errors.begin(), it.errors.end());
      for (auto &error : it.errors)
      {
        list.push_back(error.message);
      }
    }
    return list;
  }

  struct FatalErrorException : public std::exception
  {
    FatalErrorException(std::string filename__, uint16_t line__, std::string message__)
      : filename{std::move(filename__)}
      , line{line__}
      , message{std::move(message__)}
    {}
    const char *what() const noexcept override
    {
      return message.c_str();
    }
    std::string filename;
    uint16_t    line{};
    std::string message;
  };

  OperatorMap       operator_map_;
  TypeMap           type_map_;
  StringSet         type_set_;
  TypeInfoArray     type_info_array_;
  TypeInfoMap       type_info_map_;
  RegisteredTypes   registered_types_;
  FunctionInfoArray function_info_array_;
  FunctionMap       function_map_;

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
  TypePtr        fixed32_type_;
  TypePtr        fixed64_type_;
  TypePtr        fixed128_type_;
  TypePtr        string_type_;
  TypePtr        address_type_;
  TypePtr        template_parameter1_type_;
  TypePtr        template_parameter2_type_;
  TypePtr        any_type_;
  TypePtr        any_primitive_type_;
  TypePtr        any_integer_type_;
  TypePtr        array_type_;
  TypePtr        map_type_;
  TypePtr        pair_type_;
  TypePtr        sharded_state_type_;
  TypePtr        state_type_;
  TypePtr        initialiser_list_type_;

  BlockNodePtr      root_;
  std::string       filename_;
  BlockNodePtrArray blocks_;
  BlockNodePtrArray loops_;
  FunctionPtr       state_constructor_;
  FunctionPtr       sharded_state_constructor_;
  NameToTypePtrMap  state_definitions_;
  NameToTypePtrMap  contract_definitions_;
  uint16_t          num_state_definitions_{};
  uint16_t          num_contract_definitions_{};
  uint16_t          num_free_functions_{};
  uint16_t          num_user_defined_types_{};
  uint16_t          num_user_defined_instantiation_types_{};
  FunctionPtr       function_;
  NodePtr           use_any_node_;
  FileErrorsArray   file_errors_array_;

  void AddError(uint16_t line, std::string const &message);
  void AddError(std::string const &filename, uint16_t line, std::string const &message);
  void CheckLocals(uint16_t line);

  void BuildBlock(BlockNodePtr const &block_node);
  void BuildContractDefinition(BlockNodePtr const &contract_definition_node);
  void BuildStructDefinition(BlockNodePtr const &struct_definition_node);
  void BuildFreeFunctionDefinition(BlockNodePtr const &function_definition_node);

  void PreAnnotateBlock(BlockNodePtr const &block_node);
  void PreAnnotatePersistentStatement(NodePtr const &persistent_statement_node);
  void PreAnnotateContractDefinition(BlockNodePtr const &contract_definition_node);
  void PreAnnotateContractFunction(BlockNodePtr const &contract_definition_node,
                                   NodePtr const &     function_node);
  void PreAnnotateStructDefinition(BlockNodePtr const &struct_definition_node);
  void PreAnnotateMemberFunctionDefinition(BlockNodePtr const &struct_definition_node,
                                           BlockNodePtr const &function_definition_node);
  void PreAnnotateMemberVarDeclarationStatement(BlockNodePtr const &struct_definition_node,
                                                NodePtr const &     var_statement_node);
  void PreAnnotateFreeFunctionDefinition(BlockNodePtr const &function_definition_node);
  bool PreAnnotatePrototype(NodePtr const &prototype_node, ExpressionNodePtrArray &parameter_nodes,
                            TypePtrArray &parameter_types, VariablePtrArray &parameter_variables,
                            TypePtr &return_type);

  void CheckInitFunctionUnique(LedgerRestrictionMetadata const &metadata);
  bool CheckSynergeticFunctionsPresentAndUnique(LedgerRestrictionMetadata const &metadata);
  void CheckSynergeticContract(LedgerRestrictionMetadata const &metadata);

  void EnforceLedgerRestrictions(BlockNodePtr const &block_node);

  void ValidateFunctionAnnotations(NodePtr const &function_node);
  void ValidateBlock(BlockNodePtr const &block_node, LedgerRestrictionMetadata &metadata);
  void ValidateFunctionPrototype(NodePtr const &function_node, LedgerRestrictionMetadata &metadata);

  void AnnotateBlock(BlockNodePtr const &block_node);
  void AnnotateStructDefinition(BlockNodePtr const &struct_definition_node);
  void AnnotateFunctionDefinition(BlockNodePtr const &function_definition_node);
  void AnnotateWhileStatement(BlockNodePtr const &while_statement_node);
  void AnnotateForStatement(BlockNodePtr const &for_statement_node);
  void AnnotateIfStatement(NodePtr const &if_statement_node);
  void AnnotateUseStatement(BlockNodePtr const &parent_block_node,
                            NodePtr const &     use_statement_node);
  void AnnotateUseAnyStatement(BlockNodePtr const &parent_block_node,
                               NodePtr const &     use_any_statement_node);
  void AnnotateContractStatement(BlockNodePtr const &parent_block_node,
                                 NodePtr const &     contract_statement_node);
  void AnnotateLocalVarStatement(BlockNodePtr const &parent_block_node,
                                 NodePtr const &     var_statement_node);
  void AnnotateReturnStatement(NodePtr const &return_statement_node);
  void AnnotateConditionalBlock(BlockNodePtr const &conditional_block_node);
  bool AnnotateTypeExpression(ExpressionNodePtr const &node);
  bool AnnotateAssignOp(ExpressionNodePtr const &node);
  bool AnnotateInplaceArithmeticOp(ExpressionNodePtr const &node);
  bool AnnotateInplaceModuloOp(ExpressionNodePtr const &node);
  bool AnnotateLHSExpression(ExpressionNodePtr const &node, ExpressionNodePtr const &lhs);
  bool AnnotateExpression(ExpressionNodePtr const &node);
  bool InternalAnnotateExpression(ExpressionNodePtr const &node);
  bool AnnotateEqualityOp(ExpressionNodePtr const &node);
  bool AnnotateRelationalOp(ExpressionNodePtr const &node);
  bool AnnotateBinaryLogicalOp(ExpressionNodePtr const &node);
  bool AnnotateUnaryLogicalOp(ExpressionNodePtr const &node);
  bool AnnotatePrefixPostfixOp(ExpressionNodePtr const &node);
  bool AnnotateNegateOp(ExpressionNodePtr const &node);
  bool AnnotateArithmeticOp(ExpressionNodePtr const &node);
  bool AnnotateModuloOp(ExpressionNodePtr const &node);
  bool AnnotateIndexOp(ExpressionNodePtr const &node);
  bool AnnotateDotOp(ExpressionNodePtr const &node);
  bool AnnotateInvokeOp(ExpressionNodePtr const &node);
  bool AnnotateInitialiserList(ExpressionNodePtr const &node);
  bool ConvertInitialiserList(ExpressionNodePtr const &node, TypePtr const &type);
  bool ConvertInitialiserListToArray(ExpressionNodePtr const &node, TypePtr const &type);
  bool TestBlock(BlockNodePtr const &block_node) const;
  bool IsWriteable(ExpressionNodePtr const &node);
  bool AnnotateArithmetic(ExpressionNodePtr const &node, ExpressionNodePtr const &lhs,
                          ExpressionNodePtr const &rhs);

  FunctionPtr FindFunction(TypePtr const &type, FunctionGroupPtr const &function_group,
                           ExpressionNodePtrArray const &parameter_nodes);
  TypePtr     ConvertNode(ExpressionNodePtr const &node, TypePtr const &expected_type);
  TypePtr     ConvertNode(ExpressionNodePtr const &node, TypePtr const &expected_type,
                          TypePtr const &type);
  TypePtr     ResolveReturnType(TypePtr const &return_type, TypePtr const &type);
  TypePtr     FindType(ExpressionNodePtr const &node);
  SymbolPtr   FindSymbol(ExpressionNodePtr const &node);
  SymbolPtr   SearchSymbols(std::string const &name);
  void        SetVariableExpression(ExpressionNodePtr const &node, VariablePtr const &variable,
                                    TypePtr const &owner);
  void        SetLVExpression(ExpressionNodePtr const &node, TypePtr const &type);
  void        SetRVExpression(ExpressionNodePtr const &node, TypePtr const &type);
  void        SetTypeExpression(ExpressionNodePtr const &node, TypePtr const &type);
  void        SetFunctionGroupExpression(ExpressionNodePtr const &node,
                                         FunctionGroupPtr const &function_group, TypePtr const &owner);
  bool        CheckType(std::string const &type_name, TypeIndex type_index);
  void        CreatePrimitiveType(std::string const &type_name, TypeIndex type_index,
                                  bool add_to_symbol_table, TypeId type_id, TypePtr &type);
  void        CreateMetaType(std::string const &type_name, TypeIndex type_index, TypeId type_id,
                             TypePtr &type);
  void        CreateClassType(std::string const &type_name, TypeIndex type_index, TypeId type_id,
                              TypePtr &type);
  void        CreateTemplateType(std::string const &type_name, TypeIndex type_index,
                                 TypePtrArray const &allowed_types, TypeId type_id, TypePtr &type);
  void        CreateTemplateInstantiationType(TypeIndex type_index, TypePtr const &template_type,
                                              TypePtrArray const &template_parameter_types, TypeId type_id,
                                              TypePtr &type);
  void        CreateGroupType(std::string const &type_name, TypeIndex type_index,
                              TypePtrArray const &allowed_types, TypeId type_id, TypePtr &type);
  TypePtr InternalCreateTemplateInstantiationType(TypeKind type_kind, TypePtr const &template_type,
                                                  TypePtrArray const &template_parameter_types);
  void    CreateFreeFunction(std::string const &name, TypePtrArray const &parameter_types,
                             TypePtr const &return_type, Handler const &handler,
                             ChargeAmount static_charge);
  void    CreateConstructor(TypePtr const &type, TypePtrArray const &parameter_types,
                            Handler const &handler, ChargeAmount static_charge);

  void        CreateStaticMemberFunction(TypePtr const &type, std::string const &name,
                                         TypePtrArray const &parameter_types, TypePtr const &return_type,
                                         Handler const &handler, ChargeAmount static_charge);
  void        CreateMemberFunction(TypePtr const &type, std::string const &name,
                                   TypePtrArray const &parameter_types, TypePtr const &return_type,
                                   Handler const &handler, ChargeAmount static_charge);
  FunctionPtr CreateUserDefinedFreeFunction(std::string const &     name,
                                            TypePtrArray const &    parameter_types,
                                            VariablePtrArray const &parameter_variables,
                                            TypePtr const &         return_type);
  FunctionPtr CreateUserDefinedContractFunction(TypePtr const &type, std::string const &name,
                                                TypePtrArray const &    parameter_types,
                                                VariablePtrArray const &parameter_variables,
                                                TypePtr const &         return_type);
  FunctionPtr CreateUserDefinedConstructor(TypePtr const &type, TypePtrArray const &parameter_types,
                                           VariablePtrArray const &parameter_variables);
  FunctionPtr CreateUserDefinedMemberFunction(TypePtr const &type, std::string const &name,
                                              TypePtrArray const &    parameter_types,
                                              VariablePtrArray const &parameter_variables,
                                              TypePtr const &         return_type);
  void        EnableIndexOperator(TypePtr const &type, TypePtrArray const &input_types,
                                  TypePtr const &output_type, Handler const &get_handler,
                                  Handler const &set_handler, ChargeAmount get_static_charge,
                                  ChargeAmount set_static_charge);
  void        AddTypeInfo(TypeKind type_kind, std::string const &type_name, TypeId type_id,
                          TypeId template_type_id, TypeIdArray const &template_parameter_type_ids,
                          TypePtr const &type);
  void        AddFunctionInfo(FunctionPtr const &function, Handler const &handler,
                              ChargeAmount static_charge);
  std::string BuildUniqueName(TypePtr const &type, std::string const &function_name,
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

  void EnableOperator(Operators &ops, Operator op)
  {
    ops.insert(op);
  }

  bool IsOperatorEnabled(Operators const &ops, Operator op) const
  {
    return ops.find(op) != ops.end();
  }

  void EnableOperator(TypePtr const &type, Operator op)
  {
    EnableOperator(type->ops, op);
  }

  void EnableLeftOperator(TypePtr const &type, Operator op)
  {
    EnableOperator(type->left_ops, op);
  }

  void EnableRightOperator(TypePtr const &type, Operator op)
  {
    EnableOperator(type->right_ops, op);
  }

  bool IsOperatorEnabled(TypePtr const &type, Operator op) const
  {
    bool const is_instantiation = type->IsInstantiation();
    TypePtr    t                = is_instantiation ? type->template_type : type;
    return IsOperatorEnabled(t->ops, op);
  }

  bool IsLeftOperatorEnabled(TypePtr const &type, Operator op) const
  {
    bool const is_instantiation = type->IsInstantiation();
    TypePtr    t                = is_instantiation ? type->template_type : type;
    return IsOperatorEnabled(t->left_ops, op);
  }

  bool IsRightOperatorEnabled(TypePtr const &type, Operator op) const
  {
    bool const is_instantiation = type->IsInstantiation();
    TypePtr    t                = is_instantiation ? type->template_type : type;
    return IsOperatorEnabled(t->right_ops, op);
  }
};

}  // namespace vm
}  // namespace fetch
