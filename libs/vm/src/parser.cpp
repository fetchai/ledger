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
  : template_names_{"Array", "Map", "State", "ShardedState", "Pair"}
{}

void Parser::AddTemplateName(std::string name)
{
  template_names_.insert(std::move(name));
}

BlockNodePtr Parser::Parse(SourceFiles const &files, std::vector<std::string> &errors)
{
  errors_.clear();
  blocks_.clear();

  BlockNodePtr root = CreateBlockNode(NodeKind::Root, "", 0);
  blocks_.push_back({root, true});

  for (auto const &file : files)
  {
    filename_                         = file.filename;
    static const std::size_t MAX_SIZE = 128 * 1024;
    std::size_t              size     = file.source.size();
    if (size > MAX_SIZE)
    {
      std::ostringstream stream;
      stream << filename_ << ": source exceeds maximum size";
      errors_.push_back(stream.str());
      break;
    }
    Tokenise(file.source);
    index_ = -1;
    token_ = nullptr;
    groups_.clear();
    operators_.clear();
    rpn_.clear();
    infix_stack_.clear();
    BlockNodePtr file_node = CreateBlockNode(NodeKind::File, filename_, 1);
    blocks_.push_back({file_node, true});
    root->block_children.push_back(file_node);
    ParseBlock(file_node);
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
  int   value;
  do
  {
    value = yylex(&token, scanner);
    tokens_.push_back(token);
  } while (value != 0);
  if (token.kind != Token::Kind::EndOfInput)
  {
    token.kind   = Token::Kind::EndOfInput;
    token.offset = location.offset;
    token.line   = location.line;
    token.text   = "";
    tokens_.push_back(token);
  }
  yy_delete_buffer(bp, scanner);
  yylex_destroy(scanner);
}

bool Parser::IsCodeBlock(NodeKind block_kind) const
{
  return ((block_kind == NodeKind::MemberFunctionDefinition) ||
          (block_kind == NodeKind::FreeFunctionDefinition) ||
          (block_kind == NodeKind::WhileStatement) || (block_kind == NodeKind::ForStatement) ||
          (block_kind == NodeKind::If) || (block_kind == NodeKind::ElseIf) ||
          (block_kind == NodeKind::Else));
}

bool Parser::ParseBlock(BlockNodePtr const &block_node)
{
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
      child = ParseContract();
      break;
    }
    case Token::Kind::EndContract:
    {
      if (block_node->node_kind == NodeKind::ContractDefinition)
      {
        quit  = true;
        state = true;
        break;
      }
      AddError("no matching 'contract'");
      break;
    }
    /*
    // Disable structs for the moment
    case Token::Kind::Struct:
    {
      child = ParseStructDefinition();
      break;
    }
    case Token::Kind::EndStruct:
    {
      if (block_node->node_kind == NodeKind::StructDefinition)
      {
        quit  = true;
        state = true;
        break;
      }
      AddError("no matching 'struct'");
      break;
    }
    */
    case Token::Kind::AnnotationIdentifier:
    case Token::Kind::Function:
    {
      child = ParseFunction();
      break;
    }
    case Token::Kind::EndFunction:
    {
      if ((block_node->node_kind == NodeKind::MemberFunctionDefinition) ||
          (block_node->node_kind == NodeKind::FreeFunctionDefinition))
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
      if (block_node->node_kind == NodeKind::WhileStatement)
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
      if (block_node->node_kind == NodeKind::ForStatement)
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
      if ((block_node->node_kind == NodeKind::If) || (block_node->node_kind == NodeKind::ElseIf))
      {
        quit  = true;
        state = true;
        break;
      }
      AddError("no matching 'if' or 'elseif'");
      // Skip the block with error reporting switched off
      bool         is_elseif      = token_->kind == Token::Kind::ElseIf;
      NodeKind     temp_node_kind = is_elseif ? NodeKind::ElseIf : NodeKind::Else;
      BlockNodePtr temp_node      = CreateBlockNode(temp_node_kind, token_->text, token_->line);
      blocks_.push_back({temp_node, false});
      if (is_elseif)
      {
        ExpressionNodePtr expression_node = ParseConditionalExpression();
        if (!expression_node)
        {
          GoToNextStatement();
        }
      }
      ParseBlock(temp_node);
      break;
    }
    case Token::Kind::EndIf:
    {
      if ((block_node->node_kind == NodeKind::If) || (block_node->node_kind == NodeKind::ElseIf) ||
          (block_node->node_kind == NodeKind::Else))
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
      child = ParseVar();
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
      if (block_node->node_kind == NodeKind::File)
      {
        state = true;
      }
      else
      {
        AddError("expected " + block_node->text + " block terminator");
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
      block_node->block_terminator_text = token_->text;
      block_node->block_terminator_line = token_->line;
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
    block_node->block_children.push_back(std::move(child));
  } while (true);
}

NodePtr Parser::ParsePersistentStatement()
{
  Block const    block      = blocks_.back();
  NodeKind const block_kind = block.node->node_kind;
  NodePtr        persistent_statement_node =
      CreateBasicNode(NodeKind::PersistentStatement, token_->text, token_->line);
  if (block_kind != NodeKind::File)
  {
    AddError("persistent statement only permitted at topmost scope");
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

NodePtr Parser::ParseContract()
{
  Block const    block      = blocks_.back();
  NodeKind const block_kind = block.node->node_kind;
  NodePtr        child;
  if (IsCodeBlock(block_kind))
  {
    child = ParseContractStatement();
  }
  else
  {
    child = ParseContractDefinition();
  }
  return child;
}

NodePtr Parser::ParseFunction()
{
  Block const    block      = blocks_.back();
  NodeKind const block_kind = block.node->node_kind;
  NodePtr        child;
  if (block_kind == NodeKind::ContractDefinition)
  {
    child = ParseContractFunction();
  }
  else if (block_kind == NodeKind::StructDefinition)
  {
    child = ParseMemberFunctionDefinition();
  }
  else
  {
    child = ParseFreeFunctionDefinition();
  }
  return child;
}

BlockNodePtr Parser::ParseContractDefinition()
{
  Block const    block      = blocks_.back();
  NodeKind const block_kind = block.node->node_kind;
  BlockNodePtr   contract_definition_node =
      CreateBlockNode(NodeKind::ContractDefinition, token_->text, token_->line);
  blocks_.push_back({contract_definition_node, block.error_reporting_enabled});
  if (block_kind != NodeKind::File)
  {
    AddError("contract definition only permitted at topmost scope");
    blocks_.back().error_reporting_enabled = false;
  }
  Next();
  if (token_->kind == Token::Kind::Identifier)
  {
    ExpressionNodePtr contract_name_node =
        CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
    contract_definition_node->children.push_back(std::move(contract_name_node));
  }
  else
  {
    AddError("expected identifier");
  }
  bool parsed = ParseBlock(contract_definition_node);
  return parsed ? contract_definition_node : nullptr;
}

NodePtr Parser::ParseContractFunction()
{
  NodePtr annotations_node;
  if (token_->kind == Token::Kind::AnnotationIdentifier)
  {
    annotations_node = ParseAnnotations();
    if (!annotations_node)
    {
      SkipAnnotations();
      return nullptr;
    }
    if (token_->kind != Token::Kind::Function)
    {
      AddError("unexpected annotation(s)");
      Undo();
      return nullptr;
    }
  }
  NodePtr contract_function_node =
      CreateBasicNode(NodeKind::ContractFunction, token_->text, token_->line);
  // NOTE: the annotations node is legitimately null if no annotations are supplied
  contract_function_node->children.push_back(annotations_node);
  if (!ParsePrototype(contract_function_node))
  {
    return nullptr;
  }
  Next();
  if (token_->kind != Token::Kind::SemiColon)
  {
    AddError("expected ';'");
    return nullptr;
  }
  return contract_function_node;
}

BlockNodePtr Parser::ParseStructDefinition()
{
  Block const    block      = blocks_.back();
  NodeKind const block_kind = block.node->node_kind;
  BlockNodePtr   struct_definition_node =
      CreateBlockNode(NodeKind::StructDefinition, token_->text, token_->line);
  blocks_.push_back({struct_definition_node, block.error_reporting_enabled});
  if (block_kind != NodeKind::File)
  {
    AddError("struct definition only permitted at topmost scope");
    blocks_.back().error_reporting_enabled = false;
  }
  Next();
  if (token_->kind == Token::Kind::Identifier)
  {
    ExpressionNodePtr struct_name_node =
        CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
    struct_definition_node->children.push_back(std::move(struct_name_node));
  }
  else
  {
    AddError("expected identifier");
  }
  bool parsed = ParseBlock(struct_definition_node);
  return parsed ? struct_definition_node : nullptr;
}

BlockNodePtr Parser::ParseMemberFunctionDefinition()
{
  Block const block = blocks_.back();
  if (token_->kind == Token::Kind::AnnotationIdentifier)
  {
    AddError("unexpected annotation(s)");
    SkipAnnotations();
    return nullptr;
  }
  BlockNodePtr function_definition_node =
      CreateBlockNode(NodeKind::MemberFunctionDefinition, token_->text, token_->line);
  blocks_.push_back({function_definition_node, block.error_reporting_enabled});
  // NOTE: the annotations node is legitimately null because there are no annotations
  function_definition_node->children.push_back(nullptr);
  if (!ParsePrototype(function_definition_node))
  {
    GoToNextStatement();
  }
  bool parsed = ParseBlock(function_definition_node);
  return parsed ? function_definition_node : nullptr;
}

BlockNodePtr Parser::ParseFreeFunctionDefinition()
{
  Block const    block        = blocks_.back();
  NodeKind const block_kind   = block.node->node_kind;
  bool const     is_top_level = (block_kind == NodeKind::File);
  NodePtr        annotations_node;
  if (token_->kind == Token::Kind::AnnotationIdentifier)
  {
    if (!is_top_level)
    {
      AddError("unexpected annotation(s)");
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
      AddError("unexpected annotation(s)");
      Undo();
      return nullptr;
    }
  }
  BlockNodePtr function_definition_node =
      CreateBlockNode(NodeKind::FreeFunctionDefinition, token_->text, token_->line);
  blocks_.push_back({function_definition_node, block.error_reporting_enabled});
  if (!is_top_level)
  {
    AddError("function definition only permitted at topmost scope");
    blocks_.back().error_reporting_enabled = false;
  }
  // NOTE: the annotations node is legitimately null if no annotations are supplied
  function_definition_node->children.push_back(annotations_node);
  if (!ParsePrototype(function_definition_node))
  {
    GoToNextStatement();
  }
  bool parsed = ParseBlock(function_definition_node);
  return parsed ? function_definition_node : nullptr;
}

bool Parser::ParsePrototype(NodePtr const &prototype_node)
{
  Next();
  if (token_->kind != Token::Kind::Identifier)
  {
    AddError("expected identifier");
    return false;
  }
  ExpressionNodePtr function_name_node =
      CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
  prototype_node->children.push_back(function_name_node);
  Next();
  if (token_->kind != Token::Kind::LeftParenthesis)
  {
    AddError("expected '('");
    return false;
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
      return false;
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
      return false;
    }
  }
  else
  {
    Undo();
  }
  // NOTE: the return type node is legitimately null if no return type is supplied
  prototype_node->children.push_back(return_type_node);

  return true;
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
  bool first = true;
  while ((token_->kind != Token::Kind::EndOfInput) && (token_->kind != Token::Kind::SemiColon))
  {
    if (IsStatementKeyword(token_->kind))
    {
      if (!first)
      {
        Undo();
        return;
      }
    }
    Next();
    first = false;
  }
}

BlockNodePtr Parser::ParseWhileStatement()
{
  Block const    block      = blocks_.back();
  NodeKind const block_kind = block.node->node_kind;
  BlockNodePtr   while_statement_node =
      CreateBlockNode(NodeKind::WhileStatement, token_->text, token_->line);
  blocks_.push_back({while_statement_node, block.error_reporting_enabled});
  if (!IsCodeBlock(block_kind))
  {
    AddError("while loop not permitted outside of a function");
    blocks_.back().error_reporting_enabled = false;
  }
  ExpressionNodePtr expression = ParseConditionalExpression();
  if (expression)
  {
    while_statement_node->children.push_back(std::move(expression));
  }
  else
  {
    GoToNextStatement();
  }
  bool parsed = ParseBlock(while_statement_node);
  return parsed ? while_statement_node : nullptr;
}

BlockNodePtr Parser::ParseForStatement()
{
  Block const    block      = blocks_.back();
  NodeKind const block_kind = block.node->node_kind;
  BlockNodePtr   for_statement_node =
      CreateBlockNode(NodeKind::ForStatement, token_->text, token_->line);
  blocks_.push_back({for_statement_node, block.error_reporting_enabled});
  if (!IsCodeBlock(block_kind))
  {
    AddError("for loop not permitted outside of a function");
    blocks_.back().error_reporting_enabled = false;
  }
  bool ok = false;
  do
  {
    Next();
    if (token_->kind != Token::Kind::LeftParenthesis)
    {
      AddError("expected '('");
      break;
    }
    Next();
    if (token_->kind != Token::Kind::Identifier)
    {
      AddError("expected identifier");
      break;
    }
    ExpressionNodePtr identifier_node =
        CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
    for_statement_node->children.push_back(std::move(identifier_node));
    Next();
    if (token_->kind != Token::Kind::In)
    {
      AddError("expected 'in'");
      break;
    }
    ExpressionNodePtr part1 = ParseExpression();
    if (part1 == nullptr)
    {
      break;
    }
    for_statement_node->children.push_back(std::move(part1));
    Next();
    if (token_->kind != Token::Kind::Colon)
    {
      AddError("expected ':'");
      break;
    }
    ExpressionNodePtr part2 = ParseExpression();
    if (part2 == nullptr)
    {
      break;
    }
    for_statement_node->children.push_back(std::move(part2));
    Next();
    if (token_->kind == Token::Kind::Colon)
    {
      ExpressionNodePtr part3 = ParseExpression();
      if (part3 == nullptr)
      {
        break;
      }
      for_statement_node->children.push_back(std::move(part3));
      Next();
    }
    if (token_->kind != Token::Kind::RightParenthesis)
    {
      AddError("expected ')'");
      break;
    }
    ok = true;
  } while (false);
  if (!ok)
  {
    GoToNextStatement();
  }
  bool parsed = ParseBlock(for_statement_node);
  return parsed ? for_statement_node : nullptr;
}

NodePtr Parser::ParseIfStatement()
{
  Block const    block      = blocks_.back();
  NodeKind const block_kind = block.node->node_kind;
  NodePtr if_statement_node = CreateBasicNode(NodeKind::IfStatement, token_->text, token_->line);
  bool    if_statement_error_reporting_enabled = block.error_reporting_enabled;
  if (!IsCodeBlock(block_kind))
  {
    AddError("if statement not permitted outside of a function");
    if_statement_error_reporting_enabled = false;
  }
  bool ok = false;
  do
  {
    if (token_->kind == Token::Kind::If)
    {
      BlockNodePtr if_node = CreateBlockNode(NodeKind::If, token_->text, token_->line);
      blocks_.push_back({if_node, if_statement_error_reporting_enabled});
      ExpressionNodePtr expression_node = ParseConditionalExpression();
      if (expression_node)
      {
        if_node->children.push_back(std::move(expression_node));
      }
      else
      {
        GoToNextStatement();
      }
      if (!ParseBlock(if_node))
      {
        break;
      }
      if_statement_node->children.push_back(std::move(if_node));
      continue;
    }
    if (token_->kind == Token::Kind::ElseIf)
    {
      BlockNodePtr elseif_node = CreateBlockNode(NodeKind::ElseIf, token_->text, token_->line);
      blocks_.push_back({elseif_node, if_statement_error_reporting_enabled});
      ExpressionNodePtr expression_node = ParseConditionalExpression();
      if (expression_node)
      {
        elseif_node->children.push_back(std::move(expression_node));
      }
      else
      {
        GoToNextStatement();
      }
      if (!ParseBlock(elseif_node))
      {
        break;
      }
      if_statement_node->children.push_back(std::move(elseif_node));
      continue;
    }
    if (token_->kind == Token::Kind::Else)
    {
      BlockNodePtr else_node = CreateBlockNode(NodeKind::Else, token_->text, token_->line);
      blocks_.push_back({else_node, if_statement_error_reporting_enabled});
      if (!ParseBlock(else_node))
      {
        break;
      }
      if_statement_node->children.push_back(std::move(else_node));
      continue;
    }
    ok = true;
    break;
  } while (true);
  return ok ? if_statement_node : nullptr;
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
  Block const    block       = blocks_.back();
  NodeKind const block_kind  = block.node->node_kind;
  NodePtr use_statement_node = CreateBasicNode(NodeKind::UseStatement, token_->text, token_->line);
  if (!IsCodeBlock(block_kind))
  {
    AddError("use statement not permitted outside of a function");
    return nullptr;
  }
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
  if ((block_kind != NodeKind::MemberFunctionDefinition) &&
      (block_kind != NodeKind::FreeFunctionDefinition))
  {
    AddError("use-any statement only permitted at function scope");
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

NodePtr Parser::ParseVar()
{
  Block const    block      = blocks_.back();
  NodeKind const block_kind = block.node->node_kind;
  NodePtr        child;
  if (IsCodeBlock(block_kind))
  {
    child = ParseLocalVarStatement();
  }
  else if (block_kind == NodeKind::StructDefinition)
  {
    child = ParseMemberVarStatement();
  }
  else
  {
    AddError("variable declaration not permitted at this scope");
  }
  return child;
}

NodePtr Parser::ParseMemberVarStatement()
{
  NodePtr var_statement_node =
      CreateBasicNode(NodeKind::MemberVarDeclarationStatement, token_->text, token_->line);
  Next();
  if (token_->kind != Token::Kind::Identifier)
  {
    AddError("expected identifier");
    return nullptr;
  }
  ExpressionNodePtr identifier_node =
      CreateExpressionNode(NodeKind::Identifier, token_->text, token_->line);
  var_statement_node->children.push_back(std::move(identifier_node));
  Next();
  if (token_->kind != Token::Kind::Colon)
  {
    AddError("expected ':'");
    return nullptr;
  }
  ExpressionNodePtr type_node = ParseType();
  if (type_node == nullptr)
  {
    return nullptr;
  }
  var_statement_node->children.push_back(std::move(type_node));
  Next();
  if (token_->kind != Token::Kind::SemiColon)
  {
    AddError("expected ';'");
    return nullptr;
  }
  return var_statement_node;
}

NodePtr Parser::ParseLocalVarStatement()
{
  NodePtr var_statement_node =
      CreateBasicNode(NodeKind::LocalVarDeclarationStatement, token_->text, token_->line);
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
    var_statement_node->node_kind = NodeKind::LocalVarDeclarationTypelessAssignmentStatement;
  }
  else if (assign)
  {
    var_statement_node->node_kind = NodeKind::LocalVarDeclarationTypedAssignmentStatement;
  }
  return var_statement_node;
}

NodePtr Parser::ParseReturnStatement()
{
  Block const    block      = blocks_.back();
  NodeKind const block_kind = block.node->node_kind;
  NodePtr        return_statement_node =
      CreateBasicNode(NodeKind::ReturnStatement, token_->text, token_->line);
  if (!IsCodeBlock(block_kind))
  {
    AddError("return statement not permitted outside of a function");
    return nullptr;
  }
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
  Block const    block      = blocks_.back();
  NodeKind const block_kind = block.node->node_kind;
  NodePtr        break_statement_node =
      CreateBasicNode(NodeKind::BreakStatement, token_->text, token_->line);
  if (!IsCodeBlock(block_kind))
  {
    AddError("break statement not permitted outside of a function");
    return nullptr;
  }
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
  Block const    block      = blocks_.back();
  NodeKind const block_kind = block.node->node_kind;
  NodePtr        continue_statement_node =
      CreateBasicNode(NodeKind::ContinueStatement, token_->text, token_->line);
  if (!IsCodeBlock(block_kind))
  {
    AddError("continue statement not permitted outside of a function");
    return nullptr;
  }
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
  Block const    block      = blocks_.back();
  NodeKind const block_kind = block.node->node_kind;
  if (!IsCodeBlock(block_kind))
  {
    // Get the first token of the expression
    Next();
    if (token_->kind == Token::Kind::Unknown)
    {
      AddError("unrecognised token");
    }
    else if ((token_->kind == Token::Kind::UnterminatedString) ||
             (token_->kind == Token::Kind::UnterminatedComment) ||
             (token_->kind == Token::Kind::MaxLinesReached))
    {
      AddError("");
    }
    else
    {
      AddError("expression statement not permitted at topmost scope");
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

bool Parser::IsStatementKeyword(Token::Kind kind) const
{
  return (kind == Token::Kind::Persistent) || (kind == Token::Kind::Contract) ||
         (kind == Token::Kind::EndContract) || /* (kind == Token::Kind::Struct) || */
         /* (kind == Token::Kind::EndStruct) || */ (kind == Token::Kind::Function) ||
         (kind == Token::Kind::EndFunction) || (kind == Token::Kind::While) ||
         (kind == Token::Kind::EndWhile) || (kind == Token::Kind::For) ||
         (kind == Token::Kind::EndFor) || (kind == Token::Kind::If) ||
         (kind == Token::Kind::ElseIf) || (kind == Token::Kind::Else) ||
         (kind == Token::Kind::EndIf) || (kind == Token::Kind::Use) || (kind == Token::Kind::Var) ||
         (kind == Token::Kind::Return) || (kind == Token::Kind::Break) ||
         (kind == Token::Kind::Continue);
}

void Parser::GoToNextStatement()
{
  bool first = true;
  while ((token_->kind != Token::Kind::EndOfInput) && (token_->kind != Token::Kind::SemiColon))
  {
    if (IsStatementKeyword(token_->kind) || (token_->kind == Token::Kind::AnnotationIdentifier))
    {
      if (!first)
      {
        Undo();
        return;
      }
    }
    Next();
    first = false;
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

    case Token::Kind::Fixed32:
      parses = HandleLiteral(NodeKind::Fixed32);
      break;

    case Token::Kind::Fixed64:
      parses = HandleLiteral(NodeKind::Fixed64);
      break;

    case Token::Kind::Fixed128:
      parses = HandleLiteral(NodeKind::Fixed128);
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
        if (token_->kind == Token::Kind::Unknown)
        {
          AddError("unrecognised token");
        }
        else if ((token_->kind == Token::Kind::UnterminatedString) ||
                 (token_->kind == Token::Kind::UnterminatedComment) ||
                 (token_->kind == Token::Kind::MaxLinesReached))
        {
          AddError("");
        }
        else
        {
          AddError("expected expression");
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
  Block const block = blocks_.back();
  if (!block.error_reporting_enabled)
  {
    return;
  }
  std::ostringstream stream;
  stream << filename_ << ": line " << token_->line << ": ";
  if (token_->kind == Token::Kind::EndOfInput)
  {
    stream << "error: reached end-of-input";
  }
  else if (token_->kind == Token::Kind::UnterminatedString)
  {
    stream << "error: unterminated string";
  }
  else if (token_->kind == Token::Kind::UnterminatedComment)
  {
    stream << "error: unterminated comment";
  }
  else if (token_->kind == Token::Kind::MaxLinesReached)
  {
    stream << "error: reached maximum number of lines";
  }
  else
  {
    stream << "error at '" << token_->text << "'";
  }
  if (!message.empty())
  {
    stream << ", " << message;
  }
  errors_.push_back(stream.str());
}

}  // namespace vm
}  // namespace fetch
