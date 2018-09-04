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

#include "vm/opcodes.hpp"
#include "vm/token.hpp"
#include "vm/typeids.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace vm {

struct Symbol
{
  enum class Kind : uint16_t
  {
    Type,
    Variable,
    FunctionGroup
  };
  Symbol(const Kind kind__, const std::string &name__)
  {
    kind = kind__;
    name = name__;
  }
  virtual ~Symbol() = default;
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
  Kind        kind;
  std::string name;
};
using SymbolPtr = std::shared_ptr<Symbol>;

struct SymbolTable
{
  void Add(const std::string &name, const SymbolPtr &symbol)
  {
    map.insert(std::pair<std::string, SymbolPtr>(name, symbol));
  }
  SymbolPtr Find(const std::string &name)
  {
    auto it = map.find(name);
    if (it != map.end())
    {
      return it->second;
    }
    return nullptr;
  }
  std::unordered_map<std::string, SymbolPtr> map;
};
using SymbolTablePtr = std::shared_ptr<SymbolTable>;
inline SymbolTablePtr CreateSymbolTable()
{
  return std::make_shared<SymbolTable>(SymbolTable());
}

struct Type;
using TypePtr = std::shared_ptr<Type>;
struct Type : public Symbol
{
  enum class Category : uint16_t
  {
    Primitive,
    Template,
    TemplateInstantiation,
    Class
  };
  Type(const std::string &name, const Category category__, const TypeId id__)
    : Symbol(Kind::Type, name)
  {
    category = category__;
    id       = id__;
  }
  virtual ~Type() = default;
  bool IsPrimitiveType() const
  {
    return (category == Category::Primitive);
  }
  Category             category;
  TypeId               id;
  SymbolTablePtr       symbols;
  TypePtr              template_type;
  std::vector<TypePtr> template_parameter_types;
};
inline TypePtr CreateType(const std::string &name, const Type::Category category, const TypeId id)
{
  return std::make_shared<Type>(Type(name, category, id));
}
inline TypePtr ConvertToTypePtr(const SymbolPtr &symbol)
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
  Variable(const std::string &name, const Category category__)
    : Symbol(Kind::Variable, name)
  {
    category = category__;
    index    = 0;
  }
  virtual ~Variable() = default;
  Category category;
  TypePtr  type;
  Index    index;
};
using VariablePtr = std::shared_ptr<Variable>;
inline VariablePtr CreateVariable(const std::string &name, const Variable::Category category)
{
  return std::make_shared<Variable>(Variable(name, category));
}
inline VariablePtr ConvertToVariablePtr(const SymbolPtr &symbol)
{
  return std::static_pointer_cast<Variable>(symbol);
}

struct Function
{
  enum class Kind : uint16_t
  {
    UserFunction,
    OpcodeFunction,
    OpcodeTypeFunction,
    OpcodeInstanceFunction
  };
  Function(const Kind kind__, const std::string &name__)
  {
    kind   = kind__;
    name   = name__;
    opcode = Opcode::Unknown;
    index  = 0;
  }
  Kind                     kind;
  std::string              name;
  std::vector<TypePtr>     parameter_types;
  std::vector<VariablePtr> parameter_variables;
  TypePtr                  return_type;
  Opcode                   opcode;
  Index                    index;
};
using FunctionPtr = std::shared_ptr<Function>;
inline FunctionPtr CreateFunction(const Function::Kind kind, const std::string &name)
{
  return std::make_shared<Function>(Function(kind, name));
}

struct FunctionGroup : public Symbol
{
  FunctionGroup(const std::string &name)
    : Symbol(Kind::FunctionGroup, name)
  {}
  virtual ~FunctionGroup() = default;
  std::vector<FunctionPtr> functions;
};
using FunctionGroupPtr = std::shared_ptr<FunctionGroup>;
inline FunctionGroupPtr CreateFunctionGroup(const std::string &name)
{
  return std::make_shared<FunctionGroup>(FunctionGroup(name));
}
inline FunctionGroupPtr ConvertToFunctionGroupPtr(const SymbolPtr &symbol)
{
  return std::static_pointer_cast<FunctionGroup>(symbol);
}

struct Node;
using NodePtr = std::shared_ptr<Node>;
struct Node
{
  enum class Kind : uint16_t
  {
    Root,
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
    AddAssignOp,
    SubtractAssignOp,
    MultiplyAssignOp,
    DivideAssignOp,
    Identifier,
    Template,
    Integer32,
    UnsignedInteger32,
    Integer64,
    UnsignedInteger64,
    SinglePrecisionNumber,
    DoublePrecisionNumber,
    String,
    True,
    False,
    Null,
    AddOp,
    SubtractOp,
    MultiplyOp,
    DivideOp,
    UnaryPlusOp,
    UnaryMinusOp,
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
    IndexOp,
    DotOp,
    InvokeOp,
    RoundBracketGroup,
    SquareBracketGroup
  };
  class Hasher
  {
  public:
    size_t operator()(const Kind &key) const
    {
      return std::hash<uint16_t>{}((uint16_t)key);
    }
  };
  Node(Kind kind__, Token *token__)
  {
    kind  = kind__;
    token = *token__;
  }
  virtual ~Node() = default;
  Kind                 kind;
  Token                token;
  std::vector<NodePtr> children;
};

struct BlockNode : public Node
{
  BlockNode(Kind kind__, Token *token__)
    : Node(kind__, token__)
  {}
  virtual ~BlockNode() = default;
  std::vector<NodePtr> block_children;
  SymbolTablePtr       symbols;
};
using BlockNodePtr = std::shared_ptr<BlockNode>;

struct ExpressionNode : public Node
{
  enum class Category : uint16_t
  {
    Unknown,
    Variable,
    LV,
    RV,
    Type,
    Function
  };
  ExpressionNode(Kind kind__, Token *token__)
    : Node(kind__, token__)
  {
    category                     = Category::Unknown;
    function_invoked_on_instance = false;
  }
  virtual ~ExpressionNode() = default;
  Category         category;
  VariablePtr      variable;
  TypePtr          type;
  FunctionGroupPtr fg;
  bool             function_invoked_on_instance;
  FunctionPtr      function;
};
using ExpressionNodePtr = std::shared_ptr<ExpressionNode>;

inline BlockNodePtr ConvertToBlockNodePtr(const NodePtr &node)
{
  return std::static_pointer_cast<BlockNode>(node);
}

inline ExpressionNodePtr ConvertToExpressionNodePtr(const NodePtr &node)
{
  return std::static_pointer_cast<ExpressionNode>(node);
}

}  // namespace vm
}  // namespace fetch
