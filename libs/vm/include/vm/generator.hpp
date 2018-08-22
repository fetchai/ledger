#pragma once

#include "vm/defs.hpp"
#include "vm/node.hpp"

namespace fetch {
namespace vm {

class Generator
{
public:
  Generator() {}
  ~Generator() {}
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
