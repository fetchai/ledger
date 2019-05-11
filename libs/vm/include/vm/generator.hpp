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

#include "vm/defs.hpp"
#include "vm/node.hpp"

namespace fetch {
namespace vm {

class Generator
{
public:
  Generator()  = default;
  ~Generator() = default;
  void Generate(BlockNodePtr const &root, TypeInfoTable const &type_info_table,
                std::string const &name, Script &script);

private:
  struct Scope
  {
    std::vector<Index> objects;
  };

  struct Loop
  {
    int                scope_number;
    std::vector<Index> continue_pcs;
    std::vector<Index> break_pcs;
  };

  Script                                 script_;
  std::vector<Scope>                     scopes_;
  std::vector<Loop>                      loops_;
  std::unordered_map<std::string, Index> strings_map_;
  Strings                                strings_;
  Script::Function *                     function_;

  void   CreateFunctions(BlockNodePtr const &root);
  void   CreateAnnotations(NodePtr const &annotations_node, Script::Annotations &annotations);
  void   SetAnnotationLiteral(NodePtr const &node, Script::AnnotationLiteral &literal);
  void   HandleBlock(BlockNodePtr const &block);
  void   HandleFunctionDefinitionStatement(BlockNodePtr const &node);
  void   HandleWhileStatement(BlockNodePtr const &node);
  void   HandleForStatement(BlockNodePtr const &node);
  void   HandleIfStatement(NodePtr const &node);
  void   HandleVarStatement(NodePtr const &node);
  void   HandleReturnStatement(NodePtr const &node);
  void   HandleBreakStatement(NodePtr const &node);
  void   HandleContinueStatement(NodePtr const &node);
  Opcode GetArithmeticAssignmentOpcode(bool assigning_to_variable, bool lhs_is_primitive,
                                       bool rhs_is_primitive, Opcode opcode1, Opcode opcode2,
                                       Opcode opcode3, Opcode opcode4, Opcode opcode5,
                                       Opcode opcode6);
  void   HandleAssignmentStatement(ExpressionNodePtr const &node);
  void HandleAssignment(ExpressionNodePtr const &lhs, Opcode opcode, ExpressionNodePtr const &rhs);
  void HandleExpression(ExpressionNodePtr const &node);
  void HandleIdentifier(ExpressionNodePtr const &node);
  void HandleInteger32(ExpressionNodePtr const &node);
  void HandleUnsignedInteger32(ExpressionNodePtr const &node);
  void HandleInteger64(ExpressionNodePtr const &node);
  void HandleUnsignedInteger64(ExpressionNodePtr const &node);
  void HandleFloat32(ExpressionNodePtr const &node);
  void HandleFloat64(ExpressionNodePtr const &node);
  void HandleString(ExpressionNodePtr const &node);
  void HandleTrue(ExpressionNodePtr const &node);
  void HandleFalse(ExpressionNodePtr const &node);
  void HandleNull(ExpressionNodePtr const &node);
  void HandleIncDecOp(ExpressionNodePtr const &node);
  Opcode GetArithmeticOpcode(bool lhs_is_primitive, bool rhs_is_primitive, Opcode opcode1,
                             Opcode opcode2, Opcode opcode3, Opcode opcode4);
  void   HandleBinaryOp(ExpressionNodePtr const &node);
  void   HandleUnaryOp(ExpressionNodePtr const &node);
  void   HandleIndexOp(ExpressionNodePtr const &node);
  void   HandleDotOp(ExpressionNodePtr const &node);
  void   HandleInvokeOp(ExpressionNodePtr const &node);
  void   ScopeEnter();
  void   ScopeLeave(BlockNodePtr block_node);
};

}  // namespace vm
}  // namespace fetch
