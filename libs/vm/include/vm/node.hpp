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
#include <memory>

namespace fetch {
namespace vm {

enum class Operator : uint16_t
{
  Unknown = 0,
  Equal,
  NotEqual,
  LessThan,
  LessThanOrEqual,
  GreaterThan,
  GreaterThanOrEqual,
  UnaryMinus,
  Add,
  Subtract,
  Multiply,
  Divide,
  AddAssign,
  SubtractAssign,
  MultiplyAssign,
  DivideAssign
};

struct Symbol
{
  enum class Kind : uint16_t
  {
    Type,
    Variable,
    FunctionGroup
  };
  Symbol(std::string const &name__, Kind kind__)
  {
    name = name__;
    kind = kind__;
  }
  virtual ~Symbol() = default;
  virtual void Reset()
  {}
  bool IsType() const
  {
    return (kind == Kind::Type);
  }
  bool IsVariable() const
  {
    return (kind == Kind::Variable);
  }
  bool IsFunctionGroup() const
  {
    return (kind == Kind::FunctionGroup);
  }
  std::string name;
  Kind        kind;
};
using SymbolPtr = std::shared_ptr<Symbol>;

struct SymbolTable
{
  void Add(std::string const &name, SymbolPtr const &symbol)
  {
    map.insert(std::pair<std::string, SymbolPtr>(name, symbol));
  }
  SymbolPtr Find(std::string const &name)
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
struct Type : public Symbol
{
  Type(std::string const &name, TypeId id__, TypeCategory category__)
    : Symbol(name, Kind::Type)
  {
    id       = id__;
    category = category__;
  }
  virtual ~Type() = default;
  virtual void Reset() override
  {
    if (symbol_table)
    {
      symbol_table->Reset();
    }
    template_type = nullptr;
    types.clear();
    index_input_types.clear();
    index_output_type = nullptr;
  }
  TypeId         id;
  TypeCategory   category;
  SymbolTablePtr symbol_table;
  TypePtr        template_type;
  TypePtrArray   types;
  TypePtrArray   index_input_types;
  TypePtr        index_output_type;
};

inline TypePtr CreateType(std::string const &name, TypeId id, TypeCategory category)
{
  return std::make_shared<Type>(Type(name, id, category));
}
inline TypePtr ConvertToTypePtr(SymbolPtr const &symbol)
{
  return std::static_pointer_cast<Type>(symbol);
}

struct Variable : public Symbol
{
  enum class Category : uint16_t
  {
    Parameter,
    For,
    Local
  };
  Variable(std::string const &name, Category category__)
    : Symbol(name, Kind::Variable)
  {
    category = category__;
    index    = 0;
  }
  virtual ~Variable() = default;
  virtual void Reset() override
  {
    type = nullptr;
  }
  Category category;
  TypePtr  type;
  Index    index;
};
using VariablePtr      = std::shared_ptr<Variable>;
using VariablePtrArray = std::vector<VariablePtr>;

inline VariablePtr CreateVariable(std::string const &name, Variable::Category category)
{
  return std::make_shared<Variable>(Variable(name, category));
}
inline VariablePtr ConvertToVariablePtr(SymbolPtr const &symbol)
{
  return std::static_pointer_cast<Variable>(symbol);
}

struct Function
{
  enum class Kind : uint16_t
  {
    UserFreeFunction,
    OpcodeFreeFunction,
    OpcodeTypeFunction,
    OpcodeInstanceFunction
  };
  Function(std::string const &name__, Kind kind__)
  {
    name   = name__;
    kind   = kind__;
    opcode = Opcodes::Unknown;
    index  = 0;
  }
  void Reset()
  {
    parameter_types.clear();
    parameter_variables.clear();
    return_type = nullptr;
  }
  std::string      name;
  Kind             kind;
  Opcode           opcode;
  Index            index;
  TypePtrArray     parameter_types;
  VariablePtrArray parameter_variables;
  TypePtr          return_type;
};
using FunctionPtr      = std::shared_ptr<Function>;
using FunctionPtrArray = std::vector<FunctionPtr>;

inline FunctionPtr CreateFunction(std::string const &name, Function::Kind kind)
{
  return std::make_shared<Function>(Function(name, kind));
}

struct FunctionGroup : public Symbol
{
  FunctionGroup(std::string const &name)
    : Symbol(name, Kind::FunctionGroup)
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
  enum class Kind : uint16_t
  {
    Unknown = 0,
    Root,
    Annotations,
    Annotation,
    AnnotationNameValuePair,
    FunctionDefinitionStatement,
    WhileStatement,
    ForStatement,
    IfStatement,
    If,
    ElseIf,
    Else,
    VarDeclarationStatement,
    VarDeclarationTypedAssignmentStatement,
    VarDeclarationTypelessAssignmentStatement,
    ReturnStatement,
    BreakStatement,
    ContinueStatement,
    AssignOp,
    ModuloAssignOp,
    AddAssignOp,
    SubtractAssignOp,
    MultiplyAssignOp,
    DivideAssignOp,
    Identifier,
    Template,
    Integer8,
    UnsignedInteger8,
    Integer16,
    UnsignedInteger16,
    Integer32,
    UnsignedInteger32,
    Integer64,
    UnsignedInteger64,
    Float32,
    Float64,
    String,
    True,
    False,
    Null,
    EqualOp,
    NotEqualOp,
    LessThanOp,
    LessThanOrEqualOp,
    GreaterThanOp,
    GreaterThanOrEqualOp,
    AndOp,
    OrOp,
    NotOp,
    PrefixIncOp,
    PrefixDecOp,
    PostfixIncOp,
    PostfixDecOp,
    UnaryPlusOp,
    UnaryMinusOp,
    ModuloOp,
    AddOp,
    SubtractOp,
    MultiplyOp,
    DivideOp,
    IndexOp,
    DotOp,
    InvokeOp,
    ParenthesisGroup
  };
  Node(Kind kind__, Token *token__)
    : kind(kind__)
    , token(*token__)
  {}
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
  Kind         kind;
  Token        token;
  NodePtrArray children;
};

struct BlockNode : public Node
{
  BlockNode(Kind kind__, Token *token__)
    : Node(kind__, token__)
  {}
  virtual ~BlockNode() = default;
  virtual void Reset() override
  {
    Node::Reset();
    for (auto &block : block_children)
    {
      block->Reset();
    }
    if (symbol_table)
    {
      symbol_table->Reset();
    }
  }
  NodePtrArray   block_children;
  Token          block_terminator;
  SymbolTablePtr symbol_table;
};
using BlockNodePtr      = std::shared_ptr<BlockNode>;
using BlockNodePtrArray = std::vector<BlockNodePtr>;

struct ExpressionNode : public Node
{
  enum class Category : uint16_t
  {
    Unknown = 0,
    Variable,
    LV,
    RV,
    Type,
    Function
  };
  ExpressionNode(Kind kind__, Token *token__)
    : Node(kind__, token__)
  {}
  virtual ~ExpressionNode() = default;
  virtual void Reset() override
  {
    Node::Reset();
    variable = nullptr;
    type     = nullptr;
    fg       = nullptr;
    function = nullptr;
  }
  Category         category = Category::Unknown;
  VariablePtr      variable;
  TypePtr          type;
  FunctionGroupPtr fg;
  bool             function_invoked_on_instance = false;
  FunctionPtr      function;
};
using ExpressionNodePtr = std::shared_ptr<ExpressionNode>;

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
