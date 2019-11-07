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

#include "vm/parser.hpp"

// Must define YYSTYPE and YY_EXTRA_TYPE *before* including the flex-generated header
#define YYSTYPE fetch::vm::Token
#define YY_EXTRA_TYPE fetch::vm::Location *
#include "vm/tokeniser.hpp"

#include <cstddef>
#include <memory>
#include <ostream>
#include <utility>

namespace fetch {
namespace vm {

Parser::Parser()
  : template_names_{"Array", "Map", "State", "ShardedState"}
{}

void Parser::AddTemplateName(std::string const &name)
{
  template_names_.insert(name);
}

BlockNodePtr Parser::Parse(SourceFiles const &files, std::vector<std::string> &errors)
{
  errors_.clear();
  blocks_.clear();

  BlockNodePtr root = CreateBlockNode(NodeKind::Root, "", 0);

  for (auto const &file : files)
  {
    filename_ = file.filename;
    Tokenise(file.source);
    index_ = -1;
    token_ = nullptr;
    groups_.clear();
    operators_.clear();
    rpn_.clear();
    infix_stack_.clear();
    BlockNodePtr file_node = CreateBlockNode(NodeKind::File, filename_, 1);
    root->block_children.push_back(file_node);
    ParseBlock(*file_node);
  }

  bool const ok = errors_.empty();
  errors        = std::move(errors_);

  filename_.clear();
  tokens_.clear();
  groups_.clear();
  operators_.clear();
  rpn_.clear();
  infix_stack_.clear();
  blocks_.clear();

  return ok ? root : BlockNodePtr{};
}

void Parser::Tokenise(std::string const &source)
{
  tokens_.clear();
  Location location;
  yyscan_t scanner;
  yylex_init_extra(&location, &scanner);
  YY_BUFFER_STATE bp = yy_scan_string(source.data(), scanner);
  yy_switch_to_buffer(bp, scanner);
  // There is always at least one token: the last is EndOfInput
  Token token;
  do
  {
    yylex(&token, scanner);
    tokens_.push_back(token);
  } while (token.kind != Token::Kind::EndOfInput);
  yy_delete_buffer(bp, scanner);
  yylex_destroy(scanner);
}

bool Parser::ParseBlock(BlockNode &block_node)
{
  blocks_.push_back(block_node.node_kind);
  do
  {
    bool    quit  = false;
    bool    state = false;
    NodePtr child;
    Next();
    switch (token_->kind)
    {
    case Token::Kind::Persistent:
    {
      child = ParsePersistentStatement();
      break;
    }
    case Token::Kind::Contract:
    {
      if (block_node.node_kind == NodeKind::File)
      {
        child = ParseContractDefinition();
      }
      else
      {
        child = ParseContractStatement();
      }
      break;
    }
    case Token::Kind::EndContract:
    {
      AddError("no matching 'contract'");
      break;
    }
    case Token::Kind::AnnotationIdentifier:
    case Token::Kind::Function:
    {
      child = ParseFunctionDefinition();
      break;
    }
    case Token::Kind::EndFunction:
    {
      if (block_node.node_kind == NodeKind::FunctionDefinition)
      {
        quit  = true;
        state = true;
        break;
      }
      AddError("no matching 'function'");
      break;
    }
    case Token::Kind::While:
    {
      child = ParseWhileStatement();
      break;
    }
    case Token::Kind::EndWhile:
    {
      if (block_node.node_kind == NodeKind::WhileStatement)
      {
        quit  = true;
        state = true;
        break;
      }
      AddError("no matching 'while'");
      break;
    }
    case Token::Kind::For:
    {
      child = ParseForStatement();
      break;
    }
    case Token::Kind::EndFor:
    {
      if (block_node.node_kind == NodeKind::ForStatement)
      {
        quit  = true;
        state = true;
        break;
      }
      AddError("no matching 'for'");
      break;
    }
    case Token::Kind::If:
    {
      child = ParseIfStatement();
      break;
    }
    case Token::Kind::ElseIf:
    case Token::Kind::Else:
    {
      if ((block_node.node_kind == NodeKind::If) || (block_node.node_kind == NodeKind::ElseIf))
      {
        quit  = true;
        state = true;
        break;
      }
      AddError("no matching 'if' or 'elseif'");
      break;
    }
    case Token::Kind::EndIf:
    {
      if ((block_node.node_kind == NodeKind::If) || (block_node.node_kind == NodeKind::ElseIf) ||
          (block_node.node_kind == NodeKind::Else))
      {
        quit  = true;
        state = true;
        break;
      }
      AddError("no matching 'if', 'elseif' or 'else'");
      break;
    }
    case Token::Kind::Use:
    {
      child = ParseUseStatement();
      break;
    }
    case Token::Kind::Var:
    {
      child = ParseVarStatement();
      break;
    }
    case Token::Kind::Return:
    {
      child = ParseReturnStatement();
      break;
    }
    case Token::Kind::Break:
    {
      child = ParseBreakStatement();
      break;
    }
    case Token::Kind::Continue:
    {
      child = ParseContinueStatement();
      break;
    }
    case Token::Kind::EndOfInput:
    {
      quit = true;
      if (block_node.node_kind == NodeKind::File)
      {
        state = true;
      }
      else
      {
        AddError("expected statement or block terminator");
        state = false;
      }
      break;
    }
    default:
    {
      Undo();
      child = ParseExpressionStatement();
      break;
    }
    }  // switch
    if (quit)
    {
      // Store information on the block terminator
      block_node.block_terminator_text = token_->text;
      block_node.block_terminator_line = token_->line;
      blocks_.pop_back();
      return state;
    }
    if (child == nullptr)
    {
      // We got a syntax error but we want to keep on parsing to try to find
      // more syntax errors. Try to move to the start of the next
      // statement, ready to continue parsing from there.
      GoToNextStatement();
      continue;
    }
    block_node.block_children.push_back(std::move(child));
  } while (true);
}

NodePtr Parser::ParsePersistentStatement()
{
  NodePtr persistent_statement_node =
      CreateBasicNode(NodeKind::PersistentStatement, token_->text, token_->line);
  NodeKind const block_kind = blocks_.back();
  if (block_kind != NodeKind::File)
  {
    AddError("persistent statement only permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  Next();
  if (token_->kind != Token::Kind::Identifier)
  {
    AddError("expected identifier or 'sharded'");
    return nullptr;
  }
  NodePtr modifier_node;
  if (token_->text == "sharded")
  {
    modifier_node = CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
    Next();
    if (token_->kind != Token::Kind::Identifier)
    {
      AddError("expected identifier");
      return nullptr;
    }
  }
  ExpressionNodePtr state_name_node =
      CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
  Next();
  if (token_->kind != Token::Kind::Colon)
  {
    AddError("expected ':'");
    return nullptr;
  }
  ExpressionNodePtr type_node = ParseType();
  if (!type_node)
  {
    return nullptr;
  }
  Next();
  if (token_->kind != Token::Kind::SemiColon)
  {
    AddError("expected ';'");
    return nullptr;
  }
  persistent_statement_node->children.push_back(state_name_node);
  persistent_statement_node->children.push_back(modifier_node);
  persistent_statement_node->children.push_back(type_node);
  return persistent_statement_node;
}

BlockNodePtr Parser::ParseContractDefinition()
{
  BlockNodePtr contract_definition_node =
      CreateBlockNode(NodeKind::ContractDefinition, token_->text, token_->line);
  Next();
  if (token_->kind != Token::Kind::Identifier)
  {
    AddError("expected identifier");
    // Try to move to the end of the contract definition
    while ((token_->kind != Token::Kind::EndOfInput) && (token_->kind != Token::Kind::EndContract))
    {
      Next();
    }
    return nullptr;
  }
  ExpressionNodePtr contract_name_node =
      CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
  contract_definition_node->children.push_back(contract_name_node);
  blocks_.push_back(NodeKind::ContractDefinition);
  do
  {
    Next();
    if (token_->kind == Token::Kind::EndContract)
    {
      break;
    }
    if ((token_->kind != Token::Kind::AnnotationIdentifier) &&
        (token_->kind != Token::Kind::Function))
    {
      AddError("expected annotation, 'function' or 'endcontract'");
      if (token_->kind == Token::Kind::EndOfInput)
      {
        break;
      }
      SkipContractFunctionPrototype();
      continue;
    }
    NodePtr annotations_node;
    if (token_->kind == Token::Kind::AnnotationIdentifier)
    {
      annotations_node = ParseAnnotations();
      if (!annotations_node)
      {
        SkipContractFunctionPrototype();
        continue;
      }
      if (token_->kind != Token::Kind::Function)
      {
        AddError("expected 'function'");
        SkipContractFunctionPrototype();
        continue;
      }
    }
    NodePtr contract_function_prototype_node =
        CreateBasicNode(NodeKind::ContractFunctionPrototype, token_->text, token_->line);
    // NOTE: the annotations node is legitimately null if no annotations are supplied
    contract_function_prototype_node->children.push_back(annotations_node);
    if (!ParseFunctionPrototype(contract_function_prototype_node))
    {
      SkipContractFunctionPrototype();
      continue;
    }
    Next();
    if (token_->kind != Token::Kind::SemiColon)
    {
      AddError("expected ';'");
      SkipContractFunctionPrototype();
      continue;
    }
    contract_definition_node->block_children.push_back(contract_function_prototype_node);
  } while (true);
  contract_definition_node->block_terminator_text = token_->text;
  contract_definition_node->block_terminator_line = token_->line;
  blocks_.pop_back();
  return contract_definition_node;
}

void Parser::SkipContractFunctionPrototype()
{
  while ((token_->kind != Token::Kind::EndOfInput) && (token_->kind != Token::Kind::SemiColon))
  {
    if ((token_->kind == Token::Kind::Function) || (token_->kind == Token::Kind::EndContract))
    {
      Undo();
      return;
    }
    Next();
  }
}

BlockNodePtr Parser::ParseFunctionDefinition()
{
  NodeKind const block_kind   = blocks_.back();
  bool const     is_top_level = (block_kind == NodeKind::File);
  NodePtr        annotations_node;
  if (token_->kind == Token::Kind::AnnotationIdentifier)
  {
    if (!is_top_level)
    {
      AddError("unexpected annotation");
      SkipAnnotations();
      return nullptr;
    }
    annotations_node = ParseAnnotations();
    if (!annotations_node)
    {
      SkipAnnotations();
      return nullptr;
    }
    if (token_->kind != Token::Kind::Function)
    {
      AddError("expected 'function'");
      return nullptr;
    }
  }
  if (!is_top_level)
  {
    AddError("local function definitions are not permitted");
    SkipFunctionDefinition();
    return nullptr;
  }
  BlockNodePtr function_definition_node =
      CreateBlockNode(NodeKind::FunctionDefinition, token_->text, token_->line);
  // NOTE: the annotations node is legitimately null if no annotations are supplied
  function_definition_node->children.push_back(annotations_node);
  if (!ParseFunctionPrototype(function_definition_node))
  {
    SkipFunctionDefinition();
    return nullptr;
  }
  if (!ParseBlock(*function_definition_node))
  {
    return nullptr;
  }
  return function_definition_node;
}

bool Parser::ParseFunctionPrototype(NodePtr const &prototype_node)
{
  bool ok = false;
  do
  {
    Next();
    if (token_->kind != Token::Kind::Identifier)
    {
      AddError("expected identifier");
      break;
    }
    ExpressionNodePtr function_name_node =
        CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
    prototype_node->children.push_back(function_name_node);
    Next();
    if (token_->kind != Token::Kind::LeftParenthesis)
    {
      AddError("expected '('");
      break;
    }
    Next();
    if (token_->kind != Token::Kind::RightParenthesis)
    {
      bool inner_ok = false;
      int  count    = 0;
      do
      {
        if (token_->kind != Token::Kind::Identifier)
        {
          if (count != 0)
          {
            AddError("expected identifier");
          }
          else
          {
            AddError("expected identifier or ')'");
          }
          break;
        }
        ExpressionNodePtr parameter_node =
            CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
        prototype_node->children.push_back(parameter_node);
        Next();
        if (token_->kind != Token::Kind::Colon)
        {
          AddError("expected ':'");
          break;
        }
        ExpressionNodePtr parameter_type_node = ParseType();
        if (parameter_type_node == nullptr)
        {
          break;
        }
        prototype_node->children.push_back(parameter_type_node);
        Next();
        if (token_->kind == Token::Kind::RightParenthesis)
        {
          inner_ok = true;
          break;
        }
        if (token_->kind != Token::Kind::Comma)
        {
          AddError("expected ',' or ')'");
          break;
        }
        Next();
        ++count;
      } while (true);
      if (!inner_ok)
      {
        break;
      }
    }
    // Scan for optional return type
    ExpressionNodePtr return_type_node;
    Next();
    if (token_->kind == Token::Kind::Colon)
    {
      return_type_node = ParseType();
      if (return_type_node == nullptr)
      {
        break;
      }
    }
    else
    {
      Undo();
    }
    // NOTE: the return type node is legitimately null if no return type is supplied
    prototype_node->children.push_back(return_type_node);
    ok = true;
  } while (false);
  return ok;
}

void Parser::SkipFunctionDefinition()
{
  while ((token_->kind != Token::Kind::EndOfInput) && (token_->kind != Token::Kind::EndFunction))
  {
    Next();
  }
}

NodePtr Parser::ParseAnnotations()
{
  NodePtr annotations_node = CreateBasicNode(NodeKind::Annotations, token_->text, token_->line);
  do
  {
    NodePtr annotation_node = ParseAnnotation();
    if (!annotation_node)
    {
      return nullptr;
    }
    annotations_node->children.push_back(annotation_node);
  } while (token_->kind == Token::Kind::AnnotationIdentifier);
  return annotations_node;
}

NodePtr Parser::ParseAnnotation()
{
  NodePtr annotation_node = CreateBasicNode(NodeKind::Annotation, token_->text, token_->line);
  Next();
  if (token_->kind != Token::Kind::LeftParenthesis)
  {
    return annotation_node;
  }
  do
  {
    ExpressionNodePtr node = ParseAnnotationLiteral();
    if (node == nullptr)
    {
      return nullptr;
    }
    if ((node->node_kind == NodeKind::String) || (node->node_kind == NodeKind::Identifier))
    {
      Next();
      if (token_->kind != Token::Kind::Assign)
      {
        annotation_node->children.push_back(node);
      }
      else
      {
        NodePtr pair_node =
            CreateBasicNode(NodeKind::AnnotationNameValuePair, token_->text, token_->line);
        pair_node->children.push_back(node);
        ExpressionNodePtr value_node = ParseAnnotationLiteral();
        if (value_node == nullptr)
        {
          return nullptr;
        }
        pair_node->children.push_back(value_node);
        annotation_node->children.push_back(pair_node);
        Next();
      }
    }
    else
    {
      annotation_node->children.push_back(node);
      Next();
    }
    if (token_->kind == Token::Kind::RightParenthesis)
    {
      Next();
      return annotation_node;
    }
    if (token_->kind != Token::Kind::Comma)
    {
      AddError("expected ',' or ')'");
      return nullptr;
    }
  } while (true);
}

ExpressionNodePtr Parser::ParseAnnotationLiteral()
{
  NodeKind kind;
  int      sign   = 0;
  bool     number = false;
  Next();
  if (token_->kind == Token::Kind::Plus)
  {
    sign = 1;
    Next();
  }
  else if (token_->kind == Token::Kind::Minus)
  {
    sign = -1;
    Next();
  }
  switch (token_->kind)
  {
  case Token::Kind::Integer32:
  {
    kind   = NodeKind::Integer64;
    number = true;
    break;
  }
  case Token::Kind::Float64:
  {
    kind   = NodeKind::Float64;
    number = true;
    break;
  }
  case Token::Kind::True:
  {
    kind = NodeKind::True;
    break;
  }
  case Token::Kind::False:
  {
    kind = NodeKind::False;
    break;
  }
  case Token::Kind::String:
  {
    kind = NodeKind::String;
    break;
  }
  case Token::Kind::Identifier:
  {
    kind = NodeKind::Identifier;
    break;
  }
  default:
  {
    AddError("expected annotation literal");
    return nullptr;
  }
  }  // switch
  ExpressionNodePtr node = CreateExpressionNode(kind, token_->text, token_->line);
  if (sign != 0)
  {
    if (number)
    {
      if (sign == -1)
      {
        node->text = "-" + node->text;
      }
    }
    else
    {
      AddError("expected number");
      return nullptr;
    }
  }
  return node;
}

void Parser::SkipAnnotations()
{
  while ((token_->kind != Token::Kind::EndOfInput) && (token_->kind != Token::Kind::Persistent) &&
         (token_->kind != Token::Kind::Contract) && (token_->kind != Token::Kind::Function) &&
         (token_->kind != Token::Kind::While) && (token_->kind != Token::Kind::For) &&
         (token_->kind != Token::Kind::If) && (token_->kind != Token::Kind::Use) &&
         (token_->kind != Token::Kind::Var) && (token_->kind != Token::Kind::Return) &&
         (token_->kind != Token::Kind::Break) && (token_->kind != Token::Kind::Continue))
  {
    Next();
  }
}

BlockNodePtr Parser::ParseWhileStatement()
{
  NodeKind const block_kind = blocks_.back();
  if (block_kind == NodeKind::File)
  {
    AddError("while loop not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  BlockNodePtr while_statement_node =
      CreateBlockNode(NodeKind::WhileStatement, token_->text, token_->line);
  ExpressionNodePtr expression = ParseConditionalExpression();
  if (expression == nullptr)
  {
    return nullptr;
  }
  while_statement_node->children.push_back(std::move(expression));
  if (!ParseBlock(*while_statement_node))
  {
    return nullptr;
  }
  return while_statement_node;
}

BlockNodePtr Parser::ParseForStatement()
{
  NodeKind const block_kind = blocks_.back();
  if (block_kind == NodeKind::File)
  {
    AddError("for loop not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  BlockNodePtr for_statement_node =
      CreateBlockNode(NodeKind::ForStatement, token_->text, token_->line);
  Next();
  if (token_->kind != Token::Kind::LeftParenthesis)
  {
    AddError("expected '('");
    return nullptr;
  }
  Next();
  if (token_->kind != Token::Kind::Identifier)
  {
    AddError("expected identifier");
    return nullptr;
  }
  ExpressionNodePtr identifier_node =
      CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
  for_statement_node->children.push_back(std::move(identifier_node));
  Next();
  if (token_->kind != Token::Kind::In)
  {
    AddError("expected 'in'");
    return nullptr;
  }
  ExpressionNodePtr part1 = ParseExpression();
  if (part1 == nullptr)
  {
    return nullptr;
  }
  for_statement_node->children.push_back(std::move(part1));
  Next();
  if (token_->kind != Token::Kind::Colon)
  {
    AddError("expected ':'");
    return nullptr;
  }
  ExpressionNodePtr part2 = ParseExpression();
  if (part2 == nullptr)
  {
    return nullptr;
  }
  for_statement_node->children.push_back(std::move(part2));
  Next();
  if (token_->kind == Token::Kind::Colon)
  {
    ExpressionNodePtr part3 = ParseExpression();
    if (part3 == nullptr)
    {
      return nullptr;
    }
    for_statement_node->children.push_back(std::move(part3));
    Next();
  }
  if (token_->kind != Token::Kind::RightParenthesis)
  {
    AddError("expected ')'");
    return nullptr;
  }
  if (!ParseBlock(*for_statement_node))
  {
    return nullptr;
  }
  return for_statement_node;
}

NodePtr Parser::ParseIfStatement()
{
  NodeKind const block_kind = blocks_.back();
  if (block_kind == NodeKind::File)
  {
    AddError("if statement not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  NodePtr if_statement_node = CreateBasicNode(NodeKind::IfStatement, token_->text, token_->line);
  do
  {
    if (token_->kind == Token::Kind::If)
    {
      BlockNodePtr      if_node         = CreateBlockNode(NodeKind::If, token_->text, token_->line);
      ExpressionNodePtr expression_node = ParseConditionalExpression();
      if (expression_node == nullptr)
      {
        return nullptr;
      }
      if_node->children.push_back(std::move(expression_node));
      if (!ParseBlock(*if_node))
      {
        return nullptr;
      }
      if_statement_node->children.push_back(std::move(if_node));
      continue;
    }
    if (token_->kind == Token::Kind::ElseIf)
    {
      BlockNodePtr      elseif_node = CreateBlockNode(NodeKind::ElseIf, token_->text, token_->line);
      ExpressionNodePtr expression_node = ParseConditionalExpression();
      if (expression_node == nullptr)
      {
        return nullptr;
      }
      elseif_node->children.push_back(std::move(expression_node));
      if (!ParseBlock(*elseif_node))
      {
        return nullptr;
      }
      if_statement_node->children.push_back(std::move(elseif_node));
      continue;
    }
    if (token_->kind == Token::Kind::Else)
    {
      BlockNodePtr else_node = CreateBlockNode(NodeKind::Else, token_->text, token_->line);
      if (!ParseBlock(*else_node))
      {
        return nullptr;
      }
      if_statement_node->children.push_back(std::move(else_node));
      continue;
    }
    return if_statement_node;
  } while (true);
}

NodePtr Parser::ParseContractStatement()
{
  NodePtr contract_statement_node =
      CreateBasicNode(NodeKind::ContractStatement, token_->text, token_->line);
  Next();
  if (token_->kind != Token::Kind::Identifier)
  {
    AddError("expected identifier");
    return nullptr;
  }
  ExpressionNodePtr contract_variable_node =
      CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
  contract_statement_node->children.push_back(contract_variable_node);
  Next();
  if (token_->kind != Token::Kind::Assign)
  {
    AddError("expected '='");
    return nullptr;
  }
  Next();
  if (token_->kind != Token::Kind::Identifier)
  {
    AddError("expected identifier");
    return nullptr;
  }
  ExpressionNodePtr contract_type_node =
      CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
  contract_statement_node->children.push_back(contract_type_node);
  Next();
  if (token_->kind != Token::Kind::LeftParenthesis)
  {
    AddError("expected '('");
    return nullptr;
  }
  ExpressionNodePtr initialiser_node = ParseExpression();
  if (!initialiser_node)
  {
    return nullptr;
  }
  contract_statement_node->children.push_back(initialiser_node);
  Next();
  if (token_->kind != Token::Kind::RightParenthesis)
  {
    AddError("expected ')'");
    return nullptr;
  }
  Next();
  if (token_->kind != Token::Kind::SemiColon)
  {
    AddError("expected ';'");
    return nullptr;
  }
  return contract_statement_node;
}

NodePtr Parser::ParseUseStatement()
{
  NodeKind const block_kind = blocks_.back();
  if (block_kind == NodeKind::File)
  {
    AddError("use statement not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  NodePtr use_statement_node = CreateBasicNode(NodeKind::UseStatement, token_->text, token_->line);
  Next();
  if (token_->kind != Token::Kind::Identifier)
  {
    AddError("expected identifier or 'any'");
    return nullptr;
  }
  if (token_->text != "any")
  {
    ExpressionNodePtr state_name_node =
        CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
    NodePtr           list_node;
    ExpressionNodePtr alias_name_node;
    Next();
    if (token_->kind == Token::Kind::LeftSquareBracket)
    {
      list_node = CreateBasicNode(NodeKind::UseStatementKeyList, token_->text, token_->line);
      do
      {
        ExpressionNodePtr e = ParseExpression();
        if (e == nullptr)
        {
          return nullptr;
        }
        list_node->children.push_back(e);
        Next();
        if (token_->kind == Token::Kind::Comma)
        {
          continue;
        }
        if (token_->kind == Token::Kind::RightSquareBracket)
        {
          Next();
          break;
        }
        AddError("expected ',' or ']'");
        return nullptr;
      } while (true);
    }
    if (token_->kind == Token::Kind::As)
    {
      Next();
      if (token_->kind != Token::Kind::Identifier)
      {
        AddError("expected identifier");
        return nullptr;
      }
      alias_name_node = CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
      Next();
    }
    if (token_->kind != Token::Kind::SemiColon)
    {
      if (alias_name_node)
      {
        AddError("expected ';'");
      }
      else if (list_node)
      {
        AddError("expected 'as' or ';'");
      }
      else
      {
        AddError("expected '[', 'as' or ';'");
      }
      return nullptr;
    }
    use_statement_node->children.push_back(state_name_node);
    use_statement_node->children.push_back(list_node);
    use_statement_node->children.push_back(alias_name_node);
    return use_statement_node;
  }

  if (block_kind != NodeKind::FunctionDefinition)
  {
    AddError("use-any statement only permitted at function scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  use_statement_node->node_kind = NodeKind::UseAnyStatement;
  Next();
  if (token_->kind != Token::Kind::SemiColon)
  {
    AddError("expected ';'");
    return nullptr;
  }
  return use_statement_node;
}

NodePtr Parser::ParseVarStatement()
{
  NodeKind const block_kind = blocks_.back();
  if (block_kind == NodeKind::File)
  {
    AddError("variable declaration not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  NodePtr var_statement_node =
      CreateBasicNode(NodeKind::VarDeclarationStatement, token_->text, token_->line);
  Next();
  if (token_->kind != Token::Kind::Identifier)
  {
    AddError("expected identifier");
    return nullptr;
  }
  ExpressionNodePtr identifier_node =
      CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
  var_statement_node->children.push_back(std::move(identifier_node));
  bool type   = false;
  bool assign = false;
  Next();
  if (token_->kind == Token::Kind::Colon)
  {
    ExpressionNodePtr type_annotation_node = ParseType();
    if (type_annotation_node == nullptr)
    {
      return nullptr;
    }
    var_statement_node->children.push_back(std::move(type_annotation_node));
    type = true;
    Next();
  }
  if (token_->kind == Token::Kind::Assign)
  {
    ExpressionNodePtr expression_node = ParseExpression();
    if (expression_node == nullptr)
    {
      return nullptr;
    }
    var_statement_node->children.push_back(std::move(expression_node));
    assign = true;
    Next();
  }
  if (token_->kind != Token::Kind::SemiColon)
  {
    if (assign)
    {
      AddError("expected ';'");
    }
    else if (type)
    {
      AddError("expected '=' or ';'");
    }
    else
    {
      AddError("expected ':' or '='");
    }
    return nullptr;
  }
  if (!type && !assign)
  {
    AddError("expected ':' or '='");
    return nullptr;
  }
  if (!type)
  {
    var_statement_node->node_kind = NodeKind::VarDeclarationTypelessAssignmentStatement;
  }
  else if (assign)
  {
    var_statement_node->node_kind = NodeKind::VarDeclarationTypedAssignmentStatement;
  }
  return var_statement_node;
}

NodePtr Parser::ParseReturnStatement()
{
  NodeKind const block_kind = blocks_.back();
  if (block_kind == NodeKind::File)
  {
    AddError("return statement not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  NodePtr return_statement_node =
      CreateBasicNode(NodeKind::ReturnStatement, token_->text, token_->line);
  Next();
  if (token_->kind == Token::Kind::SemiColon)
  {
    return return_statement_node;
  }
  if (token_->kind == Token::Kind::EndOfInput)
  {
    AddError("expected expression or ';'");
    return nullptr;
  }
  Undo();
  ExpressionNodePtr expression = ParseExpression();
  if (expression == nullptr)
  {
    return nullptr;
  }
  Next();
  if (token_->kind != Token::Kind::SemiColon)
  {
    AddError("expected ';'");
    return nullptr;
  }
  return_statement_node->children.push_back(std::move(expression));
  return return_statement_node;
}

NodePtr Parser::ParseBreakStatement()
{
  NodeKind const block_kind = blocks_.back();
  if (block_kind == NodeKind::File)
  {
    AddError("break statement not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  NodePtr break_statement_node =
      CreateBasicNode(NodeKind::BreakStatement, token_->text, token_->line);
  Next();
  if (token_->kind != Token::Kind::SemiColon)
  {
    AddError("expected ';'");
    return nullptr;
  }
  return break_statement_node;
}

NodePtr Parser::ParseContinueStatement()
{
  NodeKind const block_kind = blocks_.back();
  if (block_kind == NodeKind::File)
  {
    AddError("continue statement not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  NodePtr continue_statement_node =
      CreateBasicNode(NodeKind::ContinueStatement, token_->text, token_->line);
  Next();
  if (token_->kind != Token::Kind::SemiColon)
  {
    AddError("expected ';'");
    return nullptr;
  }
  return continue_statement_node;
}

ExpressionNodePtr Parser::ParseExpressionStatement()
{
  NodeKind const block_kind = blocks_.back();
  if (block_kind == NodeKind::File)
  {
    // Get the first token of the expression
    Next();
    if (token_->kind != Token::Kind::Unknown)
    {
      AddError("expression statement not permitted at topmost scope");
    }
    else
    {
      AddError("unrecognised token");
    }
    return nullptr;
  }
  ExpressionNodePtr lhs = ParseExpression();
  if (lhs == nullptr)
  {
    return nullptr;
  }
  Next();
  if (token_->kind == Token::Kind::SemiColon)
  {
    // Non-assignment expression statement
    return lhs;
  }
  NodeKind kind;
  if (token_->kind == Token::Kind::Assign)
  {
    kind = NodeKind::Assign;
  }
  else if (token_->kind == Token::Kind::InplaceAdd)
  {
    kind = NodeKind::InplaceAdd;
  }
  else if (token_->kind == Token::Kind::InplaceSubtract)
  {
    kind = NodeKind::InplaceSubtract;
  }
  else if (token_->kind == Token::Kind::InplaceMultiply)
  {
    kind = NodeKind::InplaceMultiply;
  }
  else if (token_->kind == Token::Kind::InplaceDivide)
  {
    kind = NodeKind::InplaceDivide;
  }
  else if (token_->kind == Token::Kind::InplaceModulo)
  {
    kind = NodeKind::InplaceModulo;
  }
  else
  {
    AddError("expected ';' or assignment operator");
    return nullptr;
  }
  // NOTE: This doesn't handle chains of assignment a = b = c by design
  ExpressionNodePtr assignment_node = CreateExpressionNode(kind, token_->text, token_->line);
  ExpressionNodePtr rhs             = ParseExpression();
  if (rhs == nullptr)
  {
    return nullptr;
  }
  assignment_node->children.push_back(std::move(lhs));
  assignment_node->children.push_back(std::move(rhs));
  Next();
  if (token_->kind == Token::Kind::SemiColon)
  {
    return assignment_node;
  }
  AddError("expected ';'");
  return nullptr;
}

void Parser::GoToNextStatement()
{
  while ((token_->kind != Token::Kind::EndOfInput) && (token_->kind != Token::Kind::SemiColon))
  {
    if ((token_->kind == Token::Kind::Persistent) || (token_->kind == Token::Kind::Contract) ||
        (token_->kind == Token::Kind::AnnotationIdentifier) ||
        (token_->kind == Token::Kind::Function) || (token_->kind == Token::Kind::While) ||
        (token_->kind == Token::Kind::For) || (token_->kind == Token::Kind::If) ||
        (token_->kind == Token::Kind::Use) || (token_->kind == Token::Kind::Var) ||
        (token_->kind == Token::Kind::Return) || (token_->kind == Token::Kind::Break) ||
        (token_->kind == Token::Kind::Continue))
    {
      Undo();
      return;
    }
    Next();
  }
}

bool Parser::IsTemplateName(std::string const &name) const
{
  auto const it = template_names_.find(name);
  return template_names_.end() != it;
}

ExpressionNodePtr Parser::ParseType()
{
  Next();
  if (token_->kind != Token::Kind::Identifier)
  {
    AddError("expected identifier");
    return nullptr;
  }
  std::string       name = token_->text;
  ExpressionNodePtr identifier_node =
      CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
  if (!IsTemplateName(name))
  {
    return identifier_node;
  }
  Next();
  if (token_->kind != Token::Kind::LessThan)
  {
    AddError("expected '<'");
    return nullptr;
  }
  name += "<";
  ExpressionNodePtr template_node =
      CreateExpressionNode(NodeKind::Template, token_->text, token_->line);
  template_node->children.push_back(std::move(identifier_node));
  do
  {
    ExpressionNodePtr subtype_node = ParseType();
    if (subtype_node == nullptr)
    {
      return nullptr;
    }
    name += subtype_node->text;
    template_node->children.push_back(std::move(subtype_node));
    Next();
    if (token_->kind == Token::Kind::Comma)
    {
      name += ",";
      continue;
    }
    if (token_->kind == Token::Kind::GreaterThan)
    {
      name += ">";
      template_node->text = name;
      return template_node;
    }
    AddError("expected ',' or '>'");
    return nullptr;
  } while (true);
}

ExpressionNodePtr Parser::ParseConditionalExpression()
{
  Next();
  if (token_->kind != Token::Kind::LeftParenthesis)
  {
    AddError("expected '('");
    return nullptr;
  }
  Undo();
  return ParseExpression(true);
}

ExpressionNodePtr Parser::ParseExpression(bool is_conditional_expression)
{
  state_                       = State::PreOperand;
  found_expression_terminator_ = false;
  groups_.clear();
  operators_.clear();
  rpn_.clear();
  infix_stack_.clear();

  do
  {
    Next();
    bool parses{true};
    switch (token_->kind)
    {
    case Token::Kind::Identifier:
      parses = HandleIdentifier();
      break;

    case Token::Kind::Integer8:
      parses = HandleLiteral(NodeKind::Integer8);
      break;

    case Token::Kind::UnsignedInteger8:
      parses = HandleLiteral(NodeKind::UnsignedInteger8);
      break;

    case Token::Kind::Integer16:
      parses = HandleLiteral(NodeKind::Integer16);
      break;

    case Token::Kind::UnsignedInteger16:
      parses = HandleLiteral(NodeKind::UnsignedInteger16);
      break;

    case Token::Kind::Integer32:
      parses = HandleLiteral(NodeKind::Integer32);
      break;

    case Token::Kind::UnsignedInteger32:
      parses = HandleLiteral(NodeKind::UnsignedInteger32);
      break;

    case Token::Kind::Integer64:
      parses = HandleLiteral(NodeKind::Integer64);
      break;

    case Token::Kind::UnsignedInteger64:
      parses = HandleLiteral(NodeKind::UnsignedInteger64);
      break;

    case Token::Kind::Float32:
      parses = HandleLiteral(NodeKind::Float32);
      break;

    case Token::Kind::Float64:
      parses = HandleLiteral(NodeKind::Float64);
      break;

    case Token::Kind::Fixed32:
      parses = HandleLiteral(NodeKind::Fixed32);
      break;

    case Token::Kind::Fixed64:
      parses = HandleLiteral(NodeKind::Fixed64);
      break;

    case Token::Kind::String:
      parses = HandleLiteral(NodeKind::String);
      break;

    case Token::Kind::True:
      parses = HandleLiteral(NodeKind::True);
      break;

    case Token::Kind::False:
      parses = HandleLiteral(NodeKind::False);
      break;

    case Token::Kind::Null:
      parses = HandleLiteral(NodeKind::Null);
      break;

    case Token::Kind::Plus:
      HandlePlus();
      break;

    case Token::Kind::Minus:
      HandleMinus();
      break;

    case Token::Kind::Multiply:
      parses = HandleBinaryOp(NodeKind::Multiply, OpInfo(6, Association::Left, 2));
      break;

    case Token::Kind::Divide:
      parses = HandleBinaryOp(NodeKind::Divide, OpInfo(6, Association::Left, 2));
      break;

    case Token::Kind::Modulo:
      parses = HandleBinaryOp(NodeKind::Modulo, OpInfo(6, Association::Left, 2));
      break;

    case Token::Kind::Equal:
      parses = HandleBinaryOp(NodeKind::Equal, OpInfo(3, Association::Left, 2));
      break;

    case Token::Kind::NotEqual:
      parses = HandleBinaryOp(NodeKind::NotEqual, OpInfo(3, Association::Left, 2));
      break;

    case Token::Kind::LessThan:
      parses = HandleBinaryOp(NodeKind::LessThan, OpInfo(4, Association::Left, 2));
      break;

    case Token::Kind::LessThanOrEqual:
      parses = HandleBinaryOp(NodeKind::LessThanOrEqual, OpInfo(4, Association::Left, 2));
      break;

    case Token::Kind::GreaterThan:
      parses = HandleBinaryOp(NodeKind::GreaterThan, OpInfo(4, Association::Left, 2));
      break;

    case Token::Kind::GreaterThanOrEqual:
      parses = HandleBinaryOp(NodeKind::GreaterThanOrEqual, OpInfo(4, Association::Left, 2));
      break;

    case Token::Kind::And:
      parses = HandleBinaryOp(NodeKind::And, OpInfo(2, Association::Left, 2));
      break;

    case Token::Kind::Or:
      parses = HandleBinaryOp(NodeKind::Or, OpInfo(1, Association::Left, 2));
      break;

    case Token::Kind::Not:
      HandleNot();
      break;

    case Token::Kind::Inc:
      HandlePrefixPostfix(NodeKind::PrefixInc, OpInfo(7, Association::Right, 1),
                          NodeKind::PostfixInc, OpInfo(8, Association::Left, 1));
      break;

    case Token::Kind::Dec:
      HandlePrefixPostfix(NodeKind::PrefixDec, OpInfo(7, Association::Right, 1),
                          NodeKind::PostfixDec, OpInfo(8, Association::Left, 1));
      break;

    case Token::Kind::LeftParenthesis:
      parses =
          HandleOpener(NodeKind::Parenthesis, NodeKind::Invoke, Token::Kind::RightParenthesis, ")");
      break;

    case Token::Kind::LeftSquareBracket:
      parses =
          HandleOpener(NodeKind::Unknown, NodeKind::Index, Token::Kind::RightSquareBracket, "]");
      break;

    case Token::Kind::LeftBrace:
      parses =
          HandleOpener(NodeKind::InitialiserList, NodeKind::Unknown, Token::Kind::RightBrace, "}");
      break;

    case Token::Kind::RightParenthesis:
    case Token::Kind::RightSquareBracket:
    case Token::Kind::RightBrace:
      parses = HandleCloser(is_conditional_expression);
      break;

    case Token::Kind::Dot:
      parses = HandleDot();
      break;

    case Token::Kind::Comma:
      parses = HandleComma();
      break;

    default:
      if (state_ == State::PreOperand)
      {
        if (token_->kind != Token::Kind::Unknown)
        {
          AddError("expected expression");
        }
        else
        {
          AddError("unrecognised token");
        }
        return nullptr;
      }
      found_expression_terminator_ = true;
      break;

    }  // switch
    if (!parses)
    {
      return nullptr;
    }
  } while (!found_expression_terminator_);
  if (!groups_.empty())
  {
    Expr const &groupop = operators_[groups_.back()];
    AddError("expected '" + groupop.closer_token_text + "'");
    return nullptr;
  }
  // Roll back so token_ is pointing at the last token of the expression
  Undo();
  while (!operators_.empty())
  {
    Expr &topop = operators_.back();
    rpn_.push_back(std::move(topop));
    operators_.pop_back();
  }
  // rpn_ holds the Reverse Polish Notation (aka postfix) expression
  // Here we convert the RPN to an infix expression tree
  for (auto &expr : rpn_)
  {
    if ((expr.node->node_kind == NodeKind::Parenthesis) ||
        (expr.node->node_kind == NodeKind::UnaryPlus))
    {
      // Just ignore these no-ops
      continue;
    }
    if (expr.is_operator)
    {
      auto const        arity = std::size_t(expr.op_info.arity);
      std::size_t const size  = infix_stack_.size();
      for (std::size_t j = size - arity; j < size; ++j)
      {
        Expr &operand = infix_stack_[j];
        expr.node->children.push_back(std::move(operand.node));
      }
      for (std::size_t j = 0; j < arity; ++j)
      {
        infix_stack_.pop_back();
      }
      infix_stack_.push_back(std::move(expr));
    }
    else
    {
      infix_stack_.push_back(std::move(expr));
    }
  }
  // Return the root of the infix expression tree
  return {std::move(infix_stack_.front().node)};
}

bool Parser::HandleIdentifier()
{
  if (state_ == State::PostOperand)
  {
    found_expression_terminator_ = true;
    return true;
  }
  std::string name;
  if (!ParseExpressionIdentifier(name))
  {
    return false;
  }
  state_ = State::PostOperand;
  return true;
}

bool Parser::ParseExpressionIdentifier(std::string &name)
{
  AddOperand(NodeKind::Identifier);
  name = token_->text;
  if (!IsTemplateName(name))
  {
    return true;
  }
  Next();
  if (token_->kind != Token::Kind::LessThan)
  {
    AddError("expected '<'");
    return false;
  }
  name += "<";
  AddGroup(NodeKind::Template, 1, Token::Kind::GreaterThan, ">");
  do
  {
    Next();
    if (token_->kind != Token::Kind::Identifier)
    {
      AddError("expected identifier");
      return false;
    }
    std::string subtypename;
    if (!ParseExpressionIdentifier(subtypename))
    {
      return false;
    }
    name += subtypename;
    Next();
    if (token_->kind == Token::Kind::Comma)
    {
      name += ",";
      Expr &groupop = operators_.back();
      (groupop.op_info.arity)++;
      continue;
    }
    if (token_->kind == Token::Kind::GreaterThan)
    {
      name += ">";
      Expr &groupop      = operators_.back();
      groupop.node->text = name;
      (groupop.op_info.arity)++;
      groups_.pop_back();
      rpn_.push_back(std::move(groupop));
      operators_.pop_back();
      return true;
    }
    AddError("expected ',' or '>'");
    return false;
  } while (true);
}

bool Parser::HandleLiteral(NodeKind kind)
{
  if (state_ == State::PostOperand)
  {
    found_expression_terminator_ = true;
    return true;
  }
  AddOperand(kind);
  state_ = State::PostOperand;
  return true;
}

void Parser::HandlePlus()
{
  if (state_ == State::PreOperand)
  {
    HandleOp(NodeKind::UnaryPlus, OpInfo(7, Association::Right, 1));
  }
  else
  {
    HandleOp(NodeKind::Add, OpInfo(5, Association::Left, 2));
    state_ = State::PreOperand;
  }
}

void Parser::HandleMinus()
{
  if (state_ == State::PreOperand)
  {
    HandleOp(NodeKind::Negate, OpInfo(7, Association::Right, 1));
  }
  else
  {
    HandleOp(NodeKind::Subtract, OpInfo(5, Association::Left, 2));
    state_ = State::PreOperand;
  }
}

bool Parser::HandleBinaryOp(NodeKind kind, OpInfo const &op_info)
{
  if (state_ == State::PreOperand)
  {
    AddError("expected expression");
    return false;
  }
  HandleOp(kind, op_info);
  state_ = State::PreOperand;
  return true;
}

void Parser::HandleNot()
{
  if (state_ == State::PreOperand)
  {
    HandleOp(NodeKind::Not, OpInfo(7, Association::Right, 1));
  }
  else
  {
    found_expression_terminator_ = true;
  }
}

void Parser::HandlePrefixPostfix(NodeKind prefix_kind, OpInfo const &prefix_op_info,
                                 NodeKind postfix_kind, OpInfo const &postfix_op_info)
{
  if (state_ == State::PreOperand)
  {
    HandleOp(prefix_kind, prefix_op_info);
  }
  else
  {
    HandleOp(postfix_kind, postfix_op_info);
  }
}

bool Parser::HandleDot()
{
  if (state_ == State::PreOperand)
  {
    AddError("expected expression");
    return false;
  }
  ExpressionNodePtr dot_node = CreateExpressionNode(NodeKind::Dot, token_->text, token_->line);
  Next();
  if (token_->kind != Token::Kind::Identifier)
  {
    AddError("expected identifier");
    return false;
  }
  AddOperand(NodeKind::Identifier);
  IncrementGroupMembers();
  Expr expr;
  expr.is_operator       = true;
  expr.node              = std::move(dot_node);
  expr.op_info.arity     = 2;
  expr.closer_token_kind = Token::Kind::Unknown;
  expr.num_members       = 0;
  rpn_.push_back(std::move(expr));
  state_ = State::PostOperand;
  return true;
}

bool Parser::HandleOpener(NodeKind prefix_kind, NodeKind postfix_kind,
                          Token::Kind closer_token_kind, std::string const &closer_token_text)
{
  if (state_ == State::PreOperand)
  {
    if (prefix_kind == NodeKind::Unknown)
    {
      AddError("expected expression");
      return false;
    }
    AddGroup(prefix_kind, 0, closer_token_kind, closer_token_text);
    return true;
  }
  if (postfix_kind == NodeKind::Unknown)
  {
    found_expression_terminator_ = true;
    return true;
  }
  AddGroup(postfix_kind, 1, closer_token_kind, closer_token_text);
  state_ = State::PreOperand;
  return true;
}

bool Parser::HandleCloser(bool is_conditional_expression)
{
  if (groups_.empty())
  {
    // Closer without an opener
    if (state_ == State::PreOperand)
    {
      AddError("expected expression");
      return false;
    }
    found_expression_terminator_ = true;
    return true;
  }
  Expr const groupop = operators_[groups_.back()];
  if (token_->kind != groupop.closer_token_kind)
  {
    // Opener and closer don't match
    // Regardless of state_, this is a syntax error
    AddError("expected '" + groupop.closer_token_text + "'");
    return false;
  }
  while (!operators_.empty())
  {
    Expr &topop = operators_.back();
    if (topop.node->node_kind != groupop.node->node_kind)
    {
      rpn_.push_back(std::move(topop));
      operators_.pop_back();
      continue;
    }
    if (groupop.num_members > 0)
    {
      // Non-empty group
      if (state_ == State::PreOperand)
      {
        AddError("expected expression");
        return false;
      }
      (topop.op_info.arity)++;
    }
    else
    {
      // Empty group
      if ((groupop.node->node_kind == NodeKind::Parenthesis) ||
          (groupop.node->node_kind == NodeKind::Index))
      {
        AddError("expected expression");
        return false;
      }
    }
    if ((groupop.node->node_kind == NodeKind::Parenthesis) && (groups_.size() == 1) &&
        (is_conditional_expression))
    {
      // We've found the final closing bracket of a conditional expression
      Next();
      found_expression_terminator_ = true;
    }
    groups_.pop_back();
    rpn_.push_back(std::move(topop));
    operators_.pop_back();
    break;
  }
  state_ = State::PostOperand;
  return true;
}

bool Parser::HandleComma()
{
  if (state_ == State::PreOperand)
  {
    AddError("expected expression");
    return false;
  }
  if (groups_.empty())
  {
    // Commas must be inside a group
    found_expression_terminator_ = true;
    return true;
  }
  Expr const groupop = operators_[groups_.back()];
  if (groupop.node->node_kind == NodeKind::Parenthesis)
  {
    // Commas are not allowed inside a parenthesis group
    AddError("");
    return false;
  }
  while (!operators_.empty())
  {
    Expr &topop = operators_.back();
    if (topop.node->node_kind == groupop.node->node_kind)
    {
      (topop.op_info.arity)++;
      break;
    }
    rpn_.push_back(std::move(topop));
    operators_.pop_back();
  }
  state_ = State::PreOperand;
  return true;
}

void Parser::HandleOp(NodeKind kind, OpInfo const &op_info)
{
  NodeKind group_kind;
  bool     check_if_group_opener = false;
  if (!groups_.empty())
  {
    Expr const &groupop   = operators_[groups_.back()];
    group_kind            = groupop.node->node_kind;
    check_if_group_opener = true;
  }
  while (!operators_.empty())
  {
    Expr &topop = operators_.back();
    if ((check_if_group_opener) && (topop.node->node_kind == group_kind))
    {
      break;
    }
    if (op_info.precedence > topop.op_info.precedence)
    {
      break;
    }
    if (op_info.precedence == topop.op_info.precedence)
    {
      if (op_info.association == Association::Right)
      {
        break;
      }
    }
    rpn_.push_back(std::move(topop));
    operators_.pop_back();
  }
  AddOp(kind, op_info);
}

void Parser::AddGroup(NodeKind kind, int arity, Token::Kind closer_token_kind,
                      std::string const &closer_token_text)
{
  IncrementGroupMembers();
  Expr expr;
  expr.is_operator       = true;
  expr.node              = CreateExpressionNode(kind, token_->text, token_->line);
  expr.op_info.arity     = arity;
  expr.closer_token_kind = closer_token_kind;
  expr.closer_token_text = closer_token_text;
  expr.num_members       = 0;
  groups_.push_back(operators_.size());
  operators_.push_back(std::move(expr));
}

void Parser::AddOp(NodeKind kind, OpInfo const &op_info)
{
  IncrementGroupMembers();
  Expr expr;
  expr.is_operator       = true;
  expr.node              = CreateExpressionNode(kind, token_->text, token_->line);
  expr.op_info           = op_info;
  expr.closer_token_kind = Token::Kind::Unknown;
  expr.num_members       = 0;
  operators_.push_back(std::move(expr));
}

void Parser::AddOperand(NodeKind kind)
{
  IncrementGroupMembers();
  Expr expr;
  expr.is_operator       = false;
  expr.node              = CreateExpressionNode(kind, token_->text, token_->line);
  expr.closer_token_kind = Token::Kind::Unknown;
  expr.num_members       = 0;
  rpn_.push_back(std::move(expr));
}

void Parser::AddError(std::string const &message)
{
  std::ostringstream stream;
  stream << filename_ << ": line " << token_->line << ": ";
  if (token_->kind != Token::Kind::EndOfInput)
  {
    stream << "error at '" << token_->text << "'";
  }
  else
  {
    stream << "reached end-of-input";
  }
  if (!message.empty())
  {
    stream << ", " << message;
  }
  errors_.push_back(stream.str());
}

}  // namespace vm
}  // namespace fetch
