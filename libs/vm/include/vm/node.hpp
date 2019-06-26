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

#include "vm/common.hpp"
#include "vm/token.hpp"

namespace fetch {
namespace vm {

enum class Operator : uint8_t
{
  Unknown            = 0,
  Equal              = 1,
  NotEqual           = 2,
  LessThan           = 3,
  LessThanOrEqual    = 4,
  GreaterThan        = 5,
  GreaterThanOrEqual = 6,
  Negate             = 7,
  Add                = 8,
  Subtract           = 9,
  Multiply           = 10,
  Divide             = 11,
  InplaceAdd         = 12,
  InplaceSubtract    = 13,
  InplaceMultiply    = 14,
  InplaceDivide      = 15,
  ArraySeq           = 16,
  ArrayMul           = 17
};

enum class SymbolKind : uint8_t
{
  Unknown       = 0,
  Type          = 1,
  Variable      = 2,
  FunctionGroup = 3
};

struct Symbol
{
  Symbol(SymbolKind symbol_kind__, std::string const &name__)
  {
    symbol_kind = symbol_kind__;
    name        = name__;
  }
  virtual ~Symbol() = default;
  virtual void Reset()
  {}
  bool IsType() const
  {
    return (symbol_kind == SymbolKind::Type);
  }
  bool IsVariable() const
  {
    return (symbol_kind == SymbolKind::Variable);
  }
  bool IsFunctionGroup() const
  {
    return (symbol_kind == SymbolKind::FunctionGroup);
  }
  SymbolKind  symbol_kind;
  std::string name;
};
using SymbolPtr = std::shared_ptr<Symbol>;

struct SymbolTable
{
  void Add(SymbolPtr const &symbol)
  {
    map[symbol->name] = symbol;
  }
  SymbolPtr Find(std::string const &name) const
  {
    auto it = map.find(name);
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
  std::unordered_map<std::string, SymbolPtr> map;
};
using SymbolTablePtr = std::shared_ptr<SymbolTable>;

inline SymbolTablePtr CreateSymbolTable()
{
  return std::make_shared<SymbolTable>(SymbolTable());
}

struct Type;
using TypePtr      = std::shared_ptr<Type>;
using TypePtrArray = std::vector<TypePtr>;
using Operators    = std::unordered_set<Operator>;
struct Type : public Symbol
{
  Type(TypeKind type_kind__, std::string const &name)
    : Symbol(SymbolKind::Type, name)
  {
    type_kind = type_kind__;
    id        = TypeIds::Unknown;
  }
  virtual ~Type() = default;
  virtual void Reset() override
  {
    if (symbols)
    {
      symbols->Reset();
    }
    template_type = nullptr;
    types.clear();
  }
  bool IsNull() const
  {
    return (name == "Null");
  }
  bool IsVoid() const
  {
    return (name == "Void");
  }
  bool IsPrimitive() const
  {
    return (type_kind == TypeKind::Primitive);
  }
  bool IsMeta() const
  {
    return (type_kind == TypeKind::Meta);
  }
  bool IsGroup() const
  {
    return (type_kind == TypeKind::Group);
  }
  bool IsClass() const
  {
    return (type_kind == TypeKind::Class);
  }
  bool IsInstantiation() const
  {
    return ((type_kind == TypeKind::Instantiation) ||
            (type_kind == TypeKind::UserDefinedInstantiation));
  }
  TypeKind       type_kind;
  SymbolTablePtr symbols;
  TypePtr        template_type;
  TypePtrArray   types;
  Operators      ops;
  Operators      left_ops;
  Operators      right_ops;
  TypeId         id;
};

inline TypePtr CreateType(TypeKind type_kind, std::string const &name)
{
  return std::make_shared<Type>(Type(type_kind, name));
}
inline TypePtr ConvertToTypePtr(SymbolPtr const &symbol)
{
  return std::static_pointer_cast<Type>(symbol);
}

struct Variable : public Symbol
{
  Variable(VariableKind variable_kind__, std::string const &name)
    : Symbol(SymbolKind::Variable, name)
  {
    variable_kind = variable_kind__;
  }
  virtual ~Variable() = default;
  virtual void Reset() override
  {
    type = nullptr;
  }
  VariableKind variable_kind;
  TypePtr      type;
};
using VariablePtr      = std::shared_ptr<Variable>;
using VariablePtrArray = std::vector<VariablePtr>;

inline VariablePtr CreateVariable(VariableKind variable_kind, std::string const &name)
{
  return std::make_shared<Variable>(Variable(variable_kind, name));
}
inline VariablePtr ConvertToVariablePtr(SymbolPtr const &symbol)
{
  return std::static_pointer_cast<Variable>(symbol);
}

struct Function
{
  Function(FunctionKind function_kind__, std::string const &name__, std::string const &unique_id__,
           TypePtrArray const &parameter_types__, VariablePtrArray const &parameter_variables__,
           TypePtr const &return_type__)
  {
    function_kind       = function_kind__;
    name                = name__;
    unique_id           = unique_id__;
    parameter_types     = parameter_types__;
    parameter_variables = parameter_variables__;
    return_type         = return_type__;
  }
  void Reset()
  {
    parameter_types.clear();
    parameter_variables.clear();
    return_type = nullptr;
  }
  FunctionKind     function_kind;
  std::string      name;
  std::string      unique_id;
  TypePtrArray     parameter_types;
  VariablePtrArray parameter_variables;
  TypePtr          return_type;
};
using FunctionPtr      = std::shared_ptr<Function>;
using FunctionPtrArray = std::vector<FunctionPtr>;

inline FunctionPtr CreateFunction(FunctionKind function_kind, std::string const &name,
                                  std::string const &unique_id, TypePtrArray const &parameter_types,
                                  VariablePtrArray const &parameter_variables,
                                  TypePtr const &         return_type)
{
  return std::make_shared<Function>(
      Function(function_kind, name, unique_id, parameter_types, parameter_variables, return_type));
}

struct FunctionGroup : public Symbol
{
  FunctionGroup(std::string const &name)
    : Symbol(SymbolKind::FunctionGroup, name)
  {}
  virtual ~FunctionGroup() = default;
  virtual void Reset() override
  {
    for (auto &function : functions)
    {
      function->Reset();
    }
  }
  FunctionPtrArray functions;
};
using FunctionGroupPtr      = std::shared_ptr<FunctionGroup>;
using FunctionGroupPtrArray = std::vector<FunctionGroupPtr>;

inline FunctionGroupPtr CreateFunctionGroup(std::string const &name)
{
  return std::make_shared<FunctionGroup>(FunctionGroup(name));
}
inline FunctionGroupPtr ConvertToFunctionGroupPtr(SymbolPtr const &symbol)
{
  return std::static_pointer_cast<FunctionGroup>(symbol);
}

struct Node;
using NodePtr      = std::shared_ptr<Node>;
using NodePtrArray = std::vector<NodePtr>;
struct Node
{
  Node(NodeCategory node_category__, NodeKind node_kind__, std::string const &text__,
       uint16_t line__)
  {
    node_category = node_category__;
    node_kind     = node_kind__;
    text          = text__;
    line          = line__;
  }
  virtual ~Node() = default;
  virtual void Reset()
  {
    for (auto &child : children)
    {
      if (child)
      {
        child->Reset();
      }
    }
  }
  bool IsBasicNode() const
  {
    return (node_category == NodeCategory::Basic);
  }
  bool IsBlockNode() const
  {
    return (node_category == NodeCategory::Block);
  }
  bool IsExpressionNode() const
  {
    return (node_category == NodeCategory::Expression);
  }
  NodeCategory node_category;
  NodeKind     node_kind;
  std::string  text;
  uint16_t     line;
  NodePtrArray children;
};

inline NodePtr CreateBasicNode(NodeKind node_kind, std::string const &text, uint16_t line)
{
  return std::make_shared<Node>(Node(NodeCategory::Basic, node_kind, text, line));
}

struct BlockNode : public Node
{
  BlockNode(NodeKind node_kind, std::string const &text, uint16_t line)
    : Node(NodeCategory::Block, node_kind, text, line)
  {
    block_terminator_line = 0;
  }
  virtual ~BlockNode() = default;
  virtual void Reset() override
  {
    Node::Reset();
    for (auto &child : block_children)
    {
      child->Reset();
    }
    if (symbols)
    {
      symbols->Reset();
    }
  }
  NodePtrArray   block_children;
  std::string    block_terminator_text;
  uint16_t       block_terminator_line;
  SymbolTablePtr symbols;
};
using BlockNodePtr      = std::shared_ptr<BlockNode>;
using BlockNodePtrArray = std::vector<BlockNodePtr>;

inline BlockNodePtr CreateBlockNode(NodeKind node_kind, std::string const &text, uint16_t line)
{
  return std::make_shared<BlockNode>(BlockNode(node_kind, text, line));
}

struct ExpressionNode : public Node
{
  ExpressionNode(NodeKind node_kind, std::string const &text, uint16_t line)
    : Node(NodeCategory::Expression, node_kind, text, line)
  {
    expression_kind              = ExpressionKind::Unknown;
    function_invoked_on_instance = false;
  }
  virtual ~ExpressionNode() = default;
  virtual void Reset() override
  {
    Node::Reset();
    type     = nullptr;
    variable = nullptr;
    fg       = nullptr;
    function = nullptr;
  }
  bool IsVariableExpression() const
  {
    return (expression_kind == ExpressionKind::Variable);
  }
  bool IsLVExpression() const
  {
    return (expression_kind == ExpressionKind::LV);
  }
  bool IsRVExpression() const
  {
    return (expression_kind == ExpressionKind::RV);
  }
  bool IsTypeExpression() const
  {
    return (expression_kind == ExpressionKind::Type);
  }
  bool IsFunctionGroupExpression() const
  {
    return (expression_kind == ExpressionKind::FunctionGroup);
  }
  ExpressionKind   expression_kind;
  TypePtr          type;
  VariablePtr      variable;
  FunctionGroupPtr fg;
  bool             function_invoked_on_instance;
  FunctionPtr      function;
};
using ExpressionNodePtr = std::shared_ptr<ExpressionNode>;

inline ExpressionNodePtr CreateExpressionNode(NodeKind node_kind, std::string const &text,
                                              uint16_t line)
{
  return std::make_shared<ExpressionNode>(ExpressionNode(node_kind, text, line));
}

inline BlockNodePtr ConvertToBlockNodePtr(NodePtr const &node)
{
  return std::static_pointer_cast<BlockNode>(node);
}

inline ExpressionNodePtr ConvertToExpressionNodePtr(NodePtr const &node)
{
  return std::static_pointer_cast<ExpressionNode>(node);
}

}  // namespace vm
}  // namespace fetch
