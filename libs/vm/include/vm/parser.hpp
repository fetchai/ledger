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

#include "meta/type_traits.hpp"
#include "meta/type_util.hpp"
#include "meta/value_util.hpp"
#include "vm/node.hpp"

namespace fetch {
namespace vm {

class Parser
{
public:
  Parser();
  ~Parser() = default;
  BlockNodePtr Parse(std::string const &filename, std::string const &source,
                     std::vector<std::string> &errors);

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
    Token::Kind       closer_token_kind;
    std::string       closer_token_text;
    int               num_members;
  };

  using StringSet = std::unordered_set<std::string>;

  StringSet const          template_names_;
  std::vector<Token>       tokens_;
  int                      index_;
  Token *                  token_;
  std::vector<std::string> errors_;
  std::vector<NodeKind>    blocks_;
  State                    state_;
  bool                     found_expression_terminator_;
  std::vector<size_t>      groups_;
  std::vector<Expr>        operators_;
  std::vector<Expr>        rpn_;
  std::vector<Expr>        infix_stack_;

  void              Tokenise(std::string const &source);
  bool              ParseBlock(BlockNode &node);
  BlockNodePtr      ParseFunctionDefinition();
  NodePtr           ParseAnnotations();
  NodePtr           ParseAnnotation();
  ExpressionNodePtr ParseAnnotationLiteral();
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
  bool HandleSemicolon();
  bool HandleComma();
  void HandleOp(NodeKind kind, OpInfo const &op_info);
  bool HandleLeftSquareBracket();
  bool HandleArrayExpression();
  bool HandleArraySequence();
  bool HandleArrayRepetition();
  bool HandleArrayEmpty();
  void AddGroup(NodeKind kind, int arity, Token::Kind closer_token_kind,
                std::string const &closer_token_text);
  void AddOp(NodeKind kind, OpInfo const &op_info);
  void AddOperand(NodeKind kind);
  bool AddError(std::string const &message);

  bool MatchLiteral(Token::Kind token);

  template <class F>
  auto AttemptBranch(F &&f)
  {
    return std::forward<F>(f)();
  }

  template <class RV>
  auto AttemptBranch(RV (Parser::*f)())
  {
    return (this->*f)();
  }

  template <class... Fs>
  auto Either(Fs &&... fs)
  {
    using RetVal     = meta::InvokeResultT<type_util::HeadT<Fs...>>;
    const auto index = Index();
    return value_util::Accumulate(
        [this, index](RetVal accum, auto &&f) {
          if (accum)
          {
            return accum;
          }
          auto rv{AttemptBranch(std::forward<decltype(f)>(f))};
          if (!rv)
          {
            Rewind(index);
          }
          return rv;
        },
        RetVal{}, std::forward<Fs>(fs)...);
  }

  void IncrementGroupMembers()
  {
    if (groups_.size())
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

  constexpr int Index() const noexcept
  {
    return index_;
  }

  constexpr void Rewind(int index) noexcept
  {
    if (index != index_)
    {
      index_ = index;
      std::size_t i{static_cast<std::size_t>(index)};
      token_ = i < tokens_.size() ? &tokens_[i] : &tokens_.back();
    }
  }
};

}  // namespace vm
}  // namespace fetch
