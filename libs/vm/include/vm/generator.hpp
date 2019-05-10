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

#include "vm/variant.hpp"
#include "vm/ir.hpp"

namespace fetch {
namespace vm {

enum class AnnotationLiteralType : uint8_t
{
  Unknown    = 0,
  Boolean    = 1,
  Integer    = 2,
  Real       = 3,
  String     = 4,
  Identifier = 5
};

struct AnnotationLiteral
{
  AnnotationLiteral()
  {
    type = AnnotationLiteralType::Unknown;
  }
  void SetBoolean(bool b)
  {
    type    = AnnotationLiteralType::Boolean;
    boolean = b;
  }
  void SetInteger(int64_t i)
  {
    type    = AnnotationLiteralType::Integer;
    integer = i;
  }
  void SetReal(double r)
  {
    type = AnnotationLiteralType::Real;
    real = r;
  }
  void SetString(std::string const &s)
  {
    type = AnnotationLiteralType::String;
    str  = s;
  }
  void SetIdentifier(std::string const &s)
  {
    type = AnnotationLiteralType::Identifier;
    str  = s;
  }
  AnnotationLiteralType type;
  union
  {
    bool    boolean;
    int64_t integer;
    double  real;
  };
  std::string str;
};

enum class AnnotationElementType : uint8_t
{
  Unknown       = 0,
  Value         = 1,
  NameValuePair = 2
};

struct AnnotationElement
{
  AnnotationElement()
  {
    type = AnnotationElementType::Unknown;
  }
  AnnotationElementType type;
  AnnotationLiteral     name;
  AnnotationLiteral     value;
};
using AnnotationElementArray = std::vector<AnnotationElement>;

struct Annotation
{
  std::string            name;
  AnnotationElementArray elements;
};
using AnnotationArray = std::vector<Annotation>;

struct Executable
{
  Executable() = default;
  Executable(std::string const &name__)
  {
    name = name__;
  }
  ~Executable() = default;

  struct Instruction
  {
    Instruction(uint16_t opcode__)
    {
      opcode  = opcode__;
      type_id = 0;
      index   = 0;
      data    = 0;
    }
    uint16_t opcode;
    uint16_t type_id;
    uint16_t index;
    uint16_t data;
  };
  using InstructionArray = std::vector<Instruction>;

  struct Variable
  {
    Variable(std::string const &name__, TypeId type_id__, uint16_t scope_number__)
    {
      name         = name__;
      type_id      = type_id__;
      scope_number = scope_number__;
    }
    std::string name;
    TypeId      type_id;
    uint16_t    scope_number;
  };
  using VariableArray = std::vector<Variable>;

  using PcToLineMap = std::map<uint16_t, uint16_t>;
  using FunctionMap = std::unordered_map<std::string, uint16_t>;

  struct Function
  {
    Function(std::string const &name__, AnnotationArray const &annotations__, int num_parameters__,
             TypeId return_type_id__)
    {
      name           = name__;
      annotations    = annotations__;
      num_variables  = 0;
      num_parameters = num_parameters__;
      return_type_id = return_type_id__;
    }
    uint16_t AddVariable(std::string const &name, TypeId type_id, uint16_t scope_number)
    {
      uint16_t const index = uint16_t(num_variables++);
      variables.push_back(Variable(name, type_id, scope_number));
      return index;
    }
    uint16_t AddInstruction(Instruction const &instruction)
    {
      uint16_t const pc = uint16_t(instructions.size());
      instructions.push_back(instruction);
      return pc;
    }
    uint16_t FindLineNumber(uint16_t pc) const
    {
      auto it = pc_to_line_map_.lower_bound(uint16_t(pc+1));
      return (--it)->second;
    }
    std::string      name;
    AnnotationArray  annotations;
    int              num_variables;  // parameters + locals
    int              num_parameters;
    TypeId           return_type_id;
    VariableArray    variables;  // parameters + locals
    InstructionArray instructions;
    PcToLineMap      pc_to_line_map_;
  };
  using FunctionArray = std::vector<Function>;

  std::string              name;
  std::vector<std::string> strings;
  std::vector<Variant>     constants;
  TypeInfoArray            types;
  FunctionArray            functions;
  FunctionMap              function_map;

  uint16_t AddFunction(Function &function)
  {
    uint16_t const index  = uint16_t(functions.size());
    function_map[function.name] = index;
    functions.push_back(std::move(function));
    return index;
  }

  uint16_t AddType(TypeInfo const &type_info)
  {
    uint16_t const index  = uint16_t(types.size());
    types.push_back(type_info);
    return index;
  }

  Function const *FindFunction(std::string const &name) const
  {
    auto it = function_map.find(name);
    if (it != function_map.end())
    {
      return &(functions[it->second]);
    }
    return nullptr;
  }
};

class Generator
{
public:
  Generator();
  ~Generator() = default;
  bool GenerateExecutable(IR const &ir, std::string const &name, Executable &executable,
    std::vector<std::string> &errors);

private:
  struct Scope
  {
    std::vector<uint16_t> objects;
  };

  struct Loop
  {
    uint16_t              scope_number;
    std::vector<uint16_t> continue_pcs;
    std::vector<uint16_t> break_pcs;
  };

  struct ConstantComparator
  {
    bool operator()(Variant const &lhs, Variant const &rhs) const;
  };

  using StringsMap   = std::unordered_map<std::string, uint16_t>;
  using ConstantsMap = std::map<Variant, uint16_t, ConstantComparator>;
  using LineToPcMap  = std::map<uint16_t, uint16_t>;

  VM *                     vm_;
  uint16_t                 num_system_types_;
  Executable               executable_;
  std::vector<Scope>       scopes_;
  std::vector<Loop>        loops_;
  StringsMap               strings_map_;
  ConstantsMap             constants_map_;
  Executable::Function *   function_;
  LineToPcMap              line_to_pc_map_;
  std::vector<std::string> errors_;

  void   Initialise(VM *vm, uint16_t num_system_types);
  void   AddLineNumber(uint16_t line, uint16_t pc);
  void   ResolveTypes(IR const &ir);
  void   ResolveFunctions(IR const &ir);
  void   CreateFunctions(IRBlockNodePtr const &block_node);
  void   CreateAnnotations(IRNodePtr const &node, AnnotationArray &annotations);
  void   SetAnnotationLiteral(IRNodePtr const &node, AnnotationLiteral &literal);
  void   HandleBlock(IRBlockNodePtr const &block_node);
  void   HandleFile(IRBlockNodePtr const &block_node);
  void   HandleFunctionDefinitionStatement(IRBlockNodePtr const &block_node);
  void   HandleWhileStatement(IRBlockNodePtr const &block_node);
  void   HandleForStatement(IRBlockNodePtr const &block_node);
  void   HandleIfStatement(IRNodePtr const &node);
  void   HandleVarStatement(IRNodePtr const &node);
  void   HandleReturnStatement(IRNodePtr const &node);
  void   HandleBreakStatement(IRNodePtr const &node);
  void   HandleContinueStatement(IRNodePtr const &node);
  void   HandleAssignmentStatement(IRExpressionNodePtr const &node);
  void   HandleInplaceAssignmentStatement(IRExpressionNodePtr const &node);
  void HandleVariableAssignmentStatement(IRExpressionNodePtr const &lhs, IRExpressionNodePtr const &rhs);
  void HandleVariableInplaceAssignmentStatement(IRExpressionNodePtr const &node,
    IRExpressionNodePtr const &lhs, IRExpressionNodePtr const &rhs);
  void HandleIndexedAssignmentStatement(IRExpressionNodePtr const &node,
      IRExpressionNodePtr const &lhs, IRExpressionNodePtr const &rhs);
  void HandleIndexedInplaceAssignmentStatement(IRExpressionNodePtr const &node,
    IRExpressionNodePtr const &lhs, IRExpressionNodePtr const &rhs);
  void HandleExpression(IRExpressionNodePtr const &node);
  void HandleIdentifier(IRExpressionNodePtr const &node);
  void HandleInteger8(IRExpressionNodePtr const &node);
  void HandleUnsignedInteger8(IRExpressionNodePtr const &node);
  void HandleInteger16(IRExpressionNodePtr const &node);
  void HandleUnsignedInteger16(IRExpressionNodePtr const &node);
  void HandleInteger32(IRExpressionNodePtr const &node);
  void HandleUnsignedInteger32(IRExpressionNodePtr const &node);
  void HandleInteger64(IRExpressionNodePtr const &node);
  void HandleUnsignedInteger64(IRExpressionNodePtr const &node);
  void HandleFloat32(IRExpressionNodePtr const &node);
  void HandleFloat64(IRExpressionNodePtr const &node);
  void HandleString(IRExpressionNodePtr const &node);
  void HandleTrue(IRExpressionNodePtr const &node);
  void HandleFalse(IRExpressionNodePtr const &node);
  void HandleNull(IRExpressionNodePtr const &node);
  void HandlePrefixPostfixOp(IRExpressionNodePtr const &node);
  void   HandleBinaryOp(IRExpressionNodePtr const &node);
  void   HandleUnaryOp(IRExpressionNodePtr const &node);
  void   HandleIndexOp(IRExpressionNodePtr const &node);
  void   HandleDotOp(IRExpressionNodePtr const &node);
  void   HandleInvokeOp(IRExpressionNodePtr const &node);
  void     HandleVariablePrefixPostfixOp(IRExpressionNodePtr const &node, IRExpressionNodePtr const &operand);
  void     HandleIndexedPrefixPostfixOp(IRExpressionNodePtr const &node, IRExpressionNodePtr const &operand);
  void     ScopeEnter();
  void     ScopeLeave(IRBlockNodePtr const &block_node);
  uint16_t AddConstant(Variant const &c);
  uint16_t GetInplaceArithmeticOpcode(bool is_primitive,
    TypeId lhs_type_id, TypeId rhs_type_id,
    uint16_t opcode1, uint16_t opcode2, uint16_t opcode3);
  uint16_t GetArithmeticOpcode(bool lhs_is_primitive, TypeId node_type_id, TypeId lhs_type_id, TypeId rhs_type_id,
    uint16_t opcode1, uint16_t opcode2, uint16_t opcode3, uint16_t opcode4,
    TypeId &type_id, TypeId &other_type_id);

  friend class VM;
};

}  // namespace vm
}  // namespace fetch
