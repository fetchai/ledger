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

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace vm {

class Parser
{
public:
  Parser();
  ~Parser() = default;
  void         AddTemplateName(std::string name);
  BlockNodePtr Parse(SourceFiles const &files, std::vector<std::string> &errors);

private:
  enum class State
  {
    // [group opener, prefix op or operand] is required
    PreOperand,
    // [postfix op, binary op, comma or group closer] is optional
    PostOperand
  };

  enum class Association
  {
    Left,
    Right
  };

  struct OpInfo
  {
    OpInfo() = default;
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
    bool              is_operator{};
    ExpressionNodePtr node;
    OpInfo            op_info{};
    Token::Kind       closer_token_kind;
    std::string       closer_token_text;
    int               num_members{};
  };

  using StringSet = std::unordered_set<std::string>;

  struct Block
  {
    BlockNodePtr node;
    bool         error_reporting_enabled{true};
  };

  StringSet                template_names_;
  std::string              filename_;
  std::vector<Token>       tokens_;
  int                      index_{};
  Token *                  token_{};
  std::vector<std::string> errors_;
  std::vector<Block>       blocks_;
  State                    state_;
  bool                     found_expression_terminator_{};
  std::vector<std::size_t> groups_;
  std::vector<Expr>        operators_;
  std::vector<Expr>        rpn_;
  std::vector<Expr>        infix_stack_;

  void              Tokenise(std::string const &source);
  bool              IsCodeBlock(NodeKind block_kind) const;
  bool              ParseBlock(BlockNodePtr const &block_node);
  NodePtr           ParsePersistentStatement();
  NodePtr           ParseContract();
  NodePtr           ParseFunction();
  BlockNodePtr      ParseContractDefinition();
  NodePtr           ParseContractFunction();
  BlockNodePtr      ParseStructDefinition();
  BlockNodePtr      ParseMemberFunctionDefinition();
  BlockNodePtr      ParseFreeFunctionDefinition();
  bool              ParsePrototype(NodePtr const &prototype_node);
  NodePtr           ParseAnnotations();
  NodePtr           ParseAnnotation();
  ExpressionNodePtr ParseAnnotationLiteral();
  void              SkipAnnotations();
  BlockNodePtr      ParseWhileStatement();
  BlockNodePtr      ParseForStatement();
  NodePtr           ParseIfStatement();
  NodePtr           ParseContractStatement();
  NodePtr           ParseUseStatement();
  NodePtr           ParseVar();
  NodePtr           ParseMemberVarStatement();
  NodePtr           ParseLocalVarStatement();
  NodePtr           ParseReturnStatement();
  NodePtr           ParseBreakStatement();
  NodePtr           ParseContinueStatement();
  ExpressionNodePtr ParseExpressionStatement();
  bool              IsStatementKeyword(Token::Kind kind) const;
  void              GoToNextStatement();
  bool              IsTemplateName(std::string const &name) const;
  ExpressionNodePtr ParseType();
  ExpressionNodePtr ParseConditionalExpression();
  ExpressionNodePtr ParseExpression(bool is_conditional_expression = false);
  bool              HandleIdentifier();
  bool              ParseExpressionIdentifier(std::string &name);
  bool              HandleLiteral(NodeKind kind);
  void              HandlePlus();
  void              HandleMinus();
  bool              HandleBinaryOp(NodeKind kind, OpInfo const &op_info);
  void              HandleNot();
  void              HandlePrefixPostfix(NodeKind prefix_kind, OpInfo const &prefix_op_info,
                                        NodeKind postfix_kind, OpInfo const &postfix_op_info);
  bool              HandleDot();
  bool HandleOpener(NodeKind prefix_kind, NodeKind postfix_kind, Token::Kind closer_token_kind,
                    std::string const &closer_token_text);
  bool HandleCloser(bool is_conditional_expression);
  bool HandleComma();
  void HandleOp(NodeKind kind, OpInfo const &op_info);
  void AddGroup(NodeKind kind, int arity, Token::Kind closer_token_kind,
                std::string const &closer_token_text);
  void AddOp(NodeKind kind, OpInfo const &op_info);
  void AddOperand(NodeKind kind);
  void AddError(std::string const &message);

  void IncrementGroupMembers()
  {
    if (!groups_.empty())
    {
      Expr &groupop = operators_[groups_.back()];
      ++(groupop.num_members);
    }
  }

  void Next()
  {
    if (index_ < static_cast<int>(tokens_.size()) - 1)
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

}  // namespace vm
}  // namespace fetch
