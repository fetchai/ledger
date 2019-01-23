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

#include <string>  // for string

#include "vm/defs.hpp"     // for Script
#include "vm/node.hpp"     // for ExpressionNodePtr, BlockNodePtr, NodePtr
#include "vm/opcodes.hpp"  // for Opcode
#include "vm/typeids.hpp"  // for TypeId

namespace fetch {
namespace vm {

class Generator
{
public:
  Generator()
  {}
  ~Generator()
  {}
  void Generate(const BlockNodePtr &root, const std::string &name, Script &script);

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
  std::vector<std::string>               strings_;
  Script::Function *                     function_;

  void   CreateFunctions(BlockNodePtr const &root);
  void   HandleBlock(BlockNodePtr const &node);
  void   HandleFunctionDefinitionStatement(BlockNodePtr const &node);
  void   HandleWhileStatement(BlockNodePtr const &node);
  void   HandleForStatement(BlockNodePtr const &node);
  void   HandleIfStatement(NodePtr const &node);
  void   HandleVarStatement(NodePtr const &node);
  void   HandleReturnStatement(NodePtr const &node);
  void   HandleBreakStatement(NodePtr const &node);
  void   HandleContinueStatement(NodePtr const &node);
  void   HandleAssignmentStatement(ExpressionNodePtr const &node);
  void   HandleLHSExpression(ExpressionNodePtr const &lhs, Opcode const &override_opcode,
                             ExpressionNodePtr const &rhs);
  void   HandleExpression(ExpressionNodePtr const &node);
  void   HandleIdentifier(ExpressionNodePtr const &node);
  void   HandleInteger32(ExpressionNodePtr const &node);
  void   HandleUnsignedInteger32(ExpressionNodePtr const &node);
  void   HandleInteger64(ExpressionNodePtr const &node);
  void   HandleUnsignedInteger64(ExpressionNodePtr const &node);
  void   HandleSinglePrecisionNumber(ExpressionNodePtr const &node);
  void   HandleDoublePrecisionNumber(ExpressionNodePtr const &node);
  void   HandleString(ExpressionNodePtr const &node);
  void   HandleTrue(ExpressionNodePtr const &node);
  void   HandleFalse(ExpressionNodePtr const &node);
  void   HandleNull(ExpressionNodePtr const &node);
  void   HandleIncDecOp(ExpressionNodePtr const &node);
  void   HandleOp(ExpressionNodePtr const &node);
  TypeId TestArithmeticTypes(ExpressionNodePtr const &node);
  void   HandleIndexOp(ExpressionNodePtr const &node);
  void   HandleDotOp(ExpressionNodePtr const &node);
  void   HandleInvokeOp(ExpressionNodePtr const &node);
  void   ScopeEnter();
  void   ScopeLeave(BlockNodePtr block_node);
};

}  // namespace vm
}  // namespace fetch
