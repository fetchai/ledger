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

#include "vm/node.hpp"

namespace fetch {
namespace vm {

class Parser
{
public:
  Parser();
  ~Parser() = default;
  BlockNodePtr Parse(std::string const & source, Strings & errors);

private:

  enum class State
  {
    // [group opener, prefix op or operand] is required
    PreOperand,
    // [postfix op, binary op, group separator or group closer] is optional
    PostOperand
  };

  enum class Association
  {
    Left,
    Right
  };

  struct OpInfo
  {
    OpInfo()
    {}
    OpInfo(int precedence__, Association association__, int arity__)
    {
      precedence  = precedence__;
      association = association__;
      arity       = arity__;
    }
    int         precedence;
    Association association;
    int         arity;
  };

  struct Expr
  {
    bool              is_operator;
    ExpressionNodePtr node;
    OpInfo            op_info;
    int               count;
  };

  Strings                  template_names_;
  std::vector<Token>       tokens_;
  int                      index_;
  Token *                  token_;
  Strings                  errors_;
  std::vector<Node::Kind>  blocks_;
  State                    state_;
  bool                     found_expression_terminator_;
  std::vector<int>         groups_;
  std::vector<Expr>        operators_;
  std::vector<Expr>        rpn_;
  std::vector<Expr>        infix_stack_;

  void              Tokenise(std::string const & source);
  bool              ParseBlock(BlockNode & node);
  BlockNodePtr      ParseFunctionDefinition();
  BlockNodePtr      ParseWhileStatement();
  BlockNodePtr      ParseForStatement();
  NodePtr           ParseIfStatement();
  NodePtr           ParseVarStatement();
  NodePtr           ParseReturnStatement();
  NodePtr           ParseBreakStatement();
  NodePtr           ParseContinueStatement();
  ExpressionNodePtr ParseExpressionStatement();
  void              GoToNextStatement();
  void              SkipFunctionDefinition();
  bool              IsTemplateName(std::string const & name) const;
  ExpressionNodePtr ParseType();
  ExpressionNodePtr ParseConditionalExpression();
  ExpressionNodePtr ParseExpression(bool is_conditional_expression = false);
  bool              HandleIdentifier();
  bool              ParseExpressionIdentifier(std::string & name);
  bool              HandleLiteral(Node::Kind kind);
  void              HandlePlus();
  void              HandleMinus();
  bool              HandleBinaryOp(Node::Kind kind, OpInfo const & op_info);
  void              HandleNot();
  void              HandleIncDec(Node::Kind     prefix_kind,
                                 OpInfo const & prefix_op_info,
                                 Node::Kind     postfix_kind,
                                 OpInfo const & postfix_op_info);
  bool              HandleDot();
  void              HandleOpener(Node::Kind prefix_kind, Node::Kind postfix_kind);
  bool              HandleCloser(bool is_conditional_expression);
  bool              HandleComma();
  void              HandleOp(Node::Kind kind, OpInfo const & op_info);
  void              AddGroup(Node::Kind kind, int initial_arity);
  void              AddOp(Node::Kind kind, OpInfo const & op_info);
  void              AddOperand(Node::Kind kind);
  void              AddError(std::string const & message);

  void IncrementNodeCount()
  {
    if (groups_.size())
    {
      Expr &group = operators_[std::size_t(groups_.back())];
      ++(group.count);
    }
  }

  void Next()
  {
    if (index_ < (int)tokens_.size() - 1)
    {
      token_ = &tokens_[std::size_t(++index_)];
    }
  }

  void Undo()
  {
    if (--index_ >= 0)
    {
      token_ = &tokens_[std::size_t(index_)];
    }
    else
    {
      token_ = nullptr;
    }
  }
};

} // namespace vm
} // namespace fetch
