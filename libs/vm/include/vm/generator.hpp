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

#include "vm/ir.hpp"
#include "vm/variant.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
  AnnotationLiteralType type{AnnotationLiteralType::Unknown};
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
  AnnotationElementType type{AnnotationElementType::Unknown};
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
  explicit Executable(std::string name__)
    : name{std::move(name__)}
  {}
  ~Executable() = default;

  struct Instruction
  {
    explicit Instruction(uint16_t opcode__)
      : opcode{opcode__}
    {}

    uint16_t opcode{};
    uint16_t type_id{};
    uint16_t index{};
    uint16_t data{};
  };
  using InstructionArray = std::vector<Instruction>;

  struct Parameter
  {
    Parameter(std::string name__, TypeId type_id__)
      : name{std::move(name__)}
      , type_id{type_id__}
    {}
    std::string name;
    TypeId      type_id{TypeIds::Unknown};
  };
  using ParameterArray = std::vector<Parameter>;

  struct Variable : public Parameter
  {
    Variable(std::string name, TypeId type_id, uint16_t scope_number__)
      : Parameter(std::move(name), type_id)
      , scope_number{scope_number__}
    {}
    uint16_t scope_number{};
  };
  using VariableArray = std::vector<Variable>;

  using PcToLineMap = std::map<uint16_t, uint16_t>;

  struct Function
  {
    Function(std::string name__, AnnotationArray annotations__, TypeId return_type_id__)
      : name{std::move(name__)}
      , annotations{std::move(annotations__)}
      , return_type_id{return_type_id__}
    {}
    void AddParameter(std::string variable_name, TypeId type_id)
    {
      parameters.emplace_back(std::move(variable_name), type_id);
      ++num_parameters;
    }
    uint16_t AddVariable(std::string variable_name, TypeId type_id, uint16_t scope_number)
    {
      auto const id = static_cast<uint16_t>(num_variables++);
      variables.emplace_back(std::move(variable_name), type_id, scope_number);
      return id;
    }
    uint16_t AddInstruction(Instruction const &instruction)
    {
      auto const pc = static_cast<uint16_t>(instructions.size());
      instructions.push_back(instruction);
      return pc;
    }
    uint16_t FindLineNumber(uint16_t pc) const
    {
      auto it = pc_to_line_map.lower_bound(uint16_t(pc + 1));
      return (--it)->second;
    }
    std::string      name;
    AnnotationArray  annotations;
    TypeId           return_type_id{TypeIds::Unknown};
    int              num_parameters{};
    ParameterArray   parameters;
    int              num_variables{};  // parameters + locals
    VariableArray    variables;        // parameters + locals
    InstructionArray instructions;
    PcToLineMap      pc_to_line_map;
  };
  using FunctionArray = std::vector<Function>;

  struct Contract
  {
    explicit Contract(std::string name__)
      : name{std::move(name__)}
    {}
    uint16_t AddFunction(Function function)
    {
      auto const id = static_cast<uint16_t>(functions.size());
      functions.push_back(std::move(function));
      return id;
    }
    std::string   name;
    FunctionArray functions;
  };
  using ContractArray = std::vector<Contract>;

  struct LargeConstant
  {
    LargeConstant() = delete;
    LargeConstant(LargeConstant const &other)
    {
      Copy(other);
    }
    LargeConstant &operator=(LargeConstant const &other)
    {
      if (this != &other)
      {
        Copy(other);
      }
      return *this;
    }
    void Copy(const LargeConstant &other)
    {
      type_id = other.type_id;
      if (type_id == TypeIds::Fixed128)
      {
        new (&fp128) fixed_point::fp128_t(other.fp128);
      }
    }
    explicit LargeConstant(fixed_point::fp128_t const &fp128__)
    {
      type_id = TypeIds::Fixed128;
      new (&fp128) fixed_point::fp128_t(fp128__);
    }
    TypeId type_id{TypeIds::Unknown};
    union
    {
      fixed_point::fp128_t fp128;
    };
  };
  using LargeConstantArray = std::vector<LargeConstant>;

  std::string              name;
  std::vector<std::string> strings;
  VariantArray             constants;
  LargeConstantArray       large_constants;
  TypeInfoArray            types;
  ContractArray            contracts;
  FunctionArray            functions;

  void AddTypeInfo(TypeInfo type_info)
  {
    types.push_back(std::move(type_info));
  }

  uint16_t AddContract(Contract contract)
  {
    auto const id = static_cast<uint16_t>(contracts.size());
    contracts.push_back(std::move(contract));
    return id;
  }

  uint16_t AddFunction(Function function)
  {
    auto const id = static_cast<uint16_t>(functions.size());
    functions.push_back(std::move(function));
    return id;
  }

  Function const *FindFunction(std::string const &fn_name) const
  {
    for (auto const &function : functions)
    {
      if (function.name == fn_name)
      {
        return &function;
      }
    }
    return nullptr;
  }
};

class Generator
{
public:
  Generator()  = default;
  ~Generator() = default;
  bool GenerateExecutable(IR const &ir, std::string const &executable_name, Executable &executable,
                          std::vector<std::string> &errors);

private:
  struct Scope
  {
    std::vector<uint16_t> objects;
  };

  struct Loop
  {
    uint16_t              scope_number{};
    std::vector<uint16_t> continue_pcs;
    std::vector<uint16_t> break_pcs;
  };

  struct Chain
  {
    Chain() = default;
    explicit Chain(NodeKind kind__)
      : kind{kind__}
    {}

    void Append(uint16_t pc)
    {
      pcs.push_back(pc);
    }
    void Append(std::vector<uint16_t> const &other_pcs)
    {
      pcs.insert(pcs.end(), other_pcs.begin(), other_pcs.end());
    }

    NodeKind              kind{NodeKind::Unknown};
    std::vector<uint16_t> pcs;
  };

  struct ConstantComparator
  {
    bool operator()(Variant const &lhs, Variant const &rhs) const;
  };

  struct LargeConstantComparator
  {
    bool operator()(Executable::LargeConstant const &lhs,
                    Executable::LargeConstant const &rhs) const;
  };

  using StringsMap        = std::unordered_map<std::string, uint16_t>;
  using ConstantsMap      = std::map<Variant, uint16_t, ConstantComparator>;
  using LargeConstantsMap = std::map<Executable::LargeConstant, uint16_t, LargeConstantComparator>;
  using LineToPcMap       = std::map<uint16_t, uint16_t>;

  VM *                     vm_{};
  uint16_t                 num_system_types_{};
  Executable               executable_;
  std::vector<Scope>       scopes_;
  std::vector<Loop>        loops_;
  StringsMap               strings_map_;
  ConstantsMap             constants_map_;
  LargeConstantsMap        large_constants_map_;
  Executable::Function *   function_{};
  LineToPcMap              line_to_pc_map_;
  std::vector<std::string> errors_;

  void  Initialise(VM *vm, uint16_t num_system_types);
  void  AddLineNumber(uint16_t line, uint16_t pc);
  void  ResolveTypes(IR const &ir);
  void  ResolveFunctions(IR const &ir);
  void  CreateUserDefinedContracts(IRBlockNodePtr const &block_node);
  void  CreateUserDefinedFunctions(IRBlockNodePtr const &block_node);
  void  CreateAnnotations(IRNodePtr const &node, AnnotationArray &annotations);
  void  SetAnnotationLiteral(IRNodePtr const &node, AnnotationLiteral &literal);
  void  HandleBlock(IRBlockNodePtr const &block_node);
  void  HandleFile(IRBlockNodePtr const &block_node);
  void  HandleFunctionDefinition(IRBlockNodePtr const &block_node);
  void  HandleWhileStatement(IRBlockNodePtr const &block_node);
  void  HandleForStatement(IRBlockNodePtr const &block_node);
  void  HandleIfStatement(IRNodePtr const &node);
  void  HandleUseStatement(IRNodePtr const &node);
  void  HandleUseAnyStatement(IRNodePtr const &node);
  void  HandleUseVariable(std::string const &name, uint16_t line, IRExpressionNodePtr const &node);
  void  HandleContractStatement(IRNodePtr const &node);
  void  HandleVarStatement(IRNodePtr const &node);
  void  HandleReturnStatement(IRNodePtr const &node);
  void  HandleBreakStatement(IRNodePtr const &node);
  void  HandleContinueStatement(IRNodePtr const &node);
  void  HandleAssignmentStatement(IRExpressionNodePtr const &node);
  void  HandleInplaceAssignmentStatement(IRExpressionNodePtr const &node);
  void  HandleVariableAssignmentStatement(IRExpressionNodePtr const &lhs,
                                          IRExpressionNodePtr const &rhs);
  void  HandleVariableInplaceAssignmentStatement(IRExpressionNodePtr const &node,
                                                 IRExpressionNodePtr const &lhs,
                                                 IRExpressionNodePtr const &rhs);
  void  HandleIndexedAssignmentStatement(IRExpressionNodePtr const &node,
                                         IRExpressionNodePtr const &lhs,
                                         IRExpressionNodePtr const &rhs);
  void  HandleIndexedInplaceAssignmentStatement(IRExpressionNodePtr const &node,
                                                IRExpressionNodePtr const &lhs,
                                                IRExpressionNodePtr const &rhs);
  void  HandleExpression(IRExpressionNodePtr const &node);
  void  HandleIdentifier(IRExpressionNodePtr const &node);
  void  HandleInteger8(IRExpressionNodePtr const &node);
  void  HandleUnsignedInteger8(IRExpressionNodePtr const &node);
  void  HandleInteger16(IRExpressionNodePtr const &node);
  void  HandleUnsignedInteger16(IRExpressionNodePtr const &node);
  void  HandleInteger32(IRExpressionNodePtr const &node);
  void  HandleUnsignedInteger32(IRExpressionNodePtr const &node);
  void  HandleInteger64(IRExpressionNodePtr const &node);
  void  HandleUnsignedInteger64(IRExpressionNodePtr const &node);
  void  HandleFloat32(IRExpressionNodePtr const &node);
  void  HandleFloat64(IRExpressionNodePtr const &node);
  void  HandleFixed32(IRExpressionNodePtr const &node);
  void  HandleFixed64(IRExpressionNodePtr const &node);
  void  HandleFixed128(IRExpressionNodePtr const &node);
  void  HandleString(IRExpressionNodePtr const &node);
  void  PushString(std::string const &s, uint16_t line);
  void  HandleTrue(IRExpressionNodePtr const &node);
  void  HandleFalse(IRExpressionNodePtr const &node);
  void  HandleInitialiserList(IRExpressionNodePtr const &node);
  void  HandleNull(IRExpressionNodePtr const &node);
  void  HandlePrefixPostfixOp(IRExpressionNodePtr const &node);
  void  HandleBinaryOp(IRExpressionNodePtr const &node);
  void  HandleUnaryOp(IRExpressionNodePtr const &node);
  Chain HandleConditionExpression(IRBlockNodePtr const &     block_node,
                                  IRExpressionNodePtr const &node);
  Chain HandleShortCircuitOp(IRNodePtr const &parent_node, IRExpressionNodePtr const &node);
  void  FinaliseShortCircuitChain(Chain const &chain, bool is_condition_chain,
                                  uint16_t destination_pc);
  void  HandleIndexOp(IRExpressionNodePtr const &node);
  void  HandleDotOp(IRExpressionNodePtr const &node);
  void  HandleInvokeOp(IRExpressionNodePtr const &node);
  void  HandleVariablePrefixPostfixOp(IRExpressionNodePtr const &node,
                                      IRExpressionNodePtr const &operand);
  void  HandleIndexedPrefixPostfixOp(IRExpressionNodePtr const &node,
                                     IRExpressionNodePtr const &operand);
  void  ScopeEnter();
  void  ScopeLeave(IRBlockNodePtr const &block_node);
  uint16_t AddConstant(Variant const &c);
  uint16_t AddLargeConstant(Executable::LargeConstant const &c);
  uint16_t GetInplaceArithmeticOpcode(bool is_primitive, TypeId lhs_type_id, TypeId rhs_type_id,
                                      uint16_t opcode1, uint16_t opcode2, uint16_t opcode3);
  uint16_t GetArithmeticOpcode(bool lhs_is_primitive, TypeId node_type_id, TypeId lhs_type_id,
                               TypeId rhs_type_id, uint16_t opcode1, uint16_t opcode2,
                               uint16_t opcode3, uint16_t opcode4, TypeId &type_id,
                               TypeId &other_type_id);

  friend class VM;
};

}  // namespace vm
}  // namespace fetch
