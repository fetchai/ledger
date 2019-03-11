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
#include <sstream>

// Must define YYSTYPE and YY_EXTRA_TYPE *before* including the flex-generated header
#define YYSTYPE fetch::vm::Token
#define YY_EXTRA_TYPE fetch::vm::Location *
#include "vm/tokeniser.hpp"

namespace fetch {
namespace vm {

Parser::Parser()
  : template_names_{
    "Matrix",
    "Array",
    "Map",
    "State",
    "StateMap"
  }
{
}

BlockNodePtr Parser::Parse(std::string const &source, Strings &errors)
{
  Tokenise(source);

  index_ = -1;
  token_ = nullptr;
  errors_.clear();
  blocks_.clear();
  groups_.clear();
  operators_.clear();
  rpn_.clear();
  infix_stack_.clear();

  BlockNodePtr root = std::make_shared<BlockNode>(Node::Kind::Root, &tokens_[0]);
  ParseBlock(*root);
  bool const ok = errors_.size() == 0;
  errors        = std::move(errors_);

  tokens_.clear();
  blocks_.clear();
  groups_.clear();
  operators_.clear();
  rpn_.clear();
  infix_stack_.clear();

  if (ok == false)
  {
    root = nullptr;
    return nullptr;
  }
  else
  {
    return root;
  }
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

bool Parser::ParseBlock(BlockNode &node)
{
  blocks_.push_back(node.kind);
  do
  {
    bool    quit  = false;
    bool    state = false;
    NodePtr child;
    Next();
    switch (token_->kind)
    {
    case Token::Kind::AnnotationIdentifier:
    case Token::Kind::Function:
    {
      child = ParseFunctionDefinition();
      break;
    }
    case Token::Kind::EndFunction:
    {
      if (node.kind == Node::Kind::FunctionDefinitionStatement)
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
      if (node.kind == Node::Kind::WhileStatement)
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
      if (node.kind == Node::Kind::ForStatement)
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
      if ((node.kind == Node::Kind::If) || (node.kind == Node::Kind::ElseIf))
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
      if ((node.kind == Node::Kind::If) || (node.kind == Node::Kind::ElseIf) ||
          (node.kind == Node::Kind::Else))
      {
        quit  = true;
        state = true;
        break;
      }
      AddError("no matching 'if', 'elseif' or 'else'");
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
      if (node.kind == Node::Kind::Root)
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
      // Store information on the token which terminated the block
      node.block_terminator = *token_;
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
    node.block_children.push_back(std::move(child));
  } while (true);
}

BlockNodePtr Parser::ParseFunctionDefinition()
{
  NodePtr annotations_node;
  if (token_->kind == Token::Kind::AnnotationIdentifier)
  {
    annotations_node = ParseAnnotations();
    if (annotations_node)
    {
      if (token_->kind != Token::Kind::Function)
      {
        AddError("");
        annotations_node = nullptr;
      }
    }
    if (annotations_node == nullptr)
    {
      while ((token_->kind != Token::Kind::EndOfInput) && (token_->kind != Token::Kind::Function))
      {
        Next();
      }
      if (token_->kind == Token::Kind::EndOfInput)
      {
        return nullptr;
      }
    }
  }
  BlockNodePtr function_definition_node =
      std::make_shared<BlockNode>(BlockNode(Node::Kind::FunctionDefinitionStatement, token_));
  // Note: the annotations node is null if no annotations are supplied
  function_definition_node->children.push_back(annotations_node);
  bool ok = false;
  do
  {
    Node::Kind const block_kind = blocks_.back();
    if (block_kind != Node::Kind::Root)
    {
      AddError("local function definitions are not permitted");
      break;
    }
    Next();
    if (token_->kind != Token::Kind::Identifier)
    {
      AddError("expected function name");
      break;
    }
    ExpressionNodePtr identifier_node =
        std::make_shared<ExpressionNode>(ExpressionNode(Node::Kind::Identifier, token_));
    function_definition_node->children.push_back(std::move(identifier_node));
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
          if (count)
          {
            AddError("expected parameter name");
          }
          else
          {
            AddError("expected parameter name or ')'");
          }
          break;
        }
        ExpressionNodePtr parameter_node =
            std::make_shared<ExpressionNode>(ExpressionNode(Node::Kind::Identifier, token_));
        function_definition_node->children.push_back(std::move(parameter_node));
        Next();
        if (token_->kind != Token::Kind::Colon)
        {
          AddError("expected ':'");
          break;
        }
        ExpressionNodePtr type_node = ParseType();
        if (type_node == nullptr)
        {
          break;
        }
        function_definition_node->children.push_back(std::move(type_node));
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
    // Note: the return type node is null if no return type ia supplied
    function_definition_node->children.push_back(return_type_node);
    ok = true;
  } while (false);
  if (ok == false)
  {
    SkipFunctionDefinition();
    return nullptr;
  }
  if (ParseBlock(*function_definition_node) == false)
  {
    return nullptr;
  }
  return function_definition_node;
}

NodePtr Parser::ParseAnnotations()
{
  NodePtr annotations_node = std::make_shared<Node>(Node(Node::Kind::Annotations, token_));
  do
  {
    NodePtr annotation_node = ParseAnnotation();
    if (annotation_node == nullptr)
    {
      return nullptr;
    }
    annotations_node->children.push_back(annotation_node);
  } while (token_->kind == Token::Kind::AnnotationIdentifier);
  return annotations_node;
}

NodePtr Parser::ParseAnnotation()
{
  NodePtr annotation_node = std::make_shared<Node>(Node(Node::Kind::Annotation, token_));
  Next();
  if (token_->kind != Token::Kind::LeftParenthesis)
  {
    return annotation_node;
  }
  do
  {
    NodePtr node = ParseAnnotationLiteral();
    if (node == nullptr)
    {
      return nullptr;
    }
    if ((node->kind == Node::Kind::String) || (node->kind == Node::Kind::Identifier))
    {
      Next();
      if (token_->kind != Token::Kind::Assign)
      {
        annotation_node->children.push_back(node);
      }
      else
      {
        NodePtr pair_node =
            std::make_shared<Node>(Node(Node::Kind::AnnotationNameValuePair, token_));
        pair_node->children.push_back(node);
        NodePtr value_node = ParseAnnotationLiteral();
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

NodePtr Parser::ParseAnnotationLiteral()
{
  Node::Kind kind;
  int        sign   = 0;
  bool       number = false;
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
    kind   = Node::Kind::Integer64;
    number = true;
    break;
  }
  case Token::Kind::Float64:
  {
    kind   = Node::Kind::Float64;
    number = true;
    break;
  }
  case Token::Kind::True:
  {
    kind = Node::Kind::True;
    break;
  }
  case Token::Kind::False:
  {
    kind = Node::Kind::False;
    break;
  }
  case Token::Kind::String:
  {
    kind = Node::Kind::String;
    break;
  }
  case Token::Kind::Identifier:
  {
    kind = Node::Kind::Identifier;
    break;
  }
  default:
  {
    AddError("expected annotation literal");
    return nullptr;
  }
  }  // switch
  NodePtr node = std::make_shared<Node>(Node(kind, token_));
  if (sign != 0)
  {
    if (number)
    {
      if (sign == -1)
      {
        node->token.text = "-" + node->token.text;
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

BlockNodePtr Parser::ParseWhileStatement()
{
  Node::Kind const block_kind = blocks_.back();
  if (block_kind == Node::Kind::Root)
  {
    AddError("while loop not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  BlockNodePtr while_statement_node =
      std::make_shared<BlockNode>(BlockNode(Node::Kind::WhileStatement, token_));
  ExpressionNodePtr expression = ParseConditionalExpression();
  if (expression == nullptr)
  {
    return nullptr;
  }
  while_statement_node->children.push_back(std::move(expression));
  if (ParseBlock(*while_statement_node) == false)
  {
    return nullptr;
  }
  return while_statement_node;
}

BlockNodePtr Parser::ParseForStatement()
{
  Node::Kind const block_kind = blocks_.back();
  if (block_kind == Node::Kind::Root)
  {
    AddError("for loop not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  BlockNodePtr for_statement_node =
      std::make_shared<BlockNode>(BlockNode(Node::Kind::ForStatement, token_));
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
      std::make_shared<ExpressionNode>(ExpressionNode(Node::Kind::Identifier, token_));
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
  if (ParseBlock(*for_statement_node) == false)
  {
    return nullptr;
  }
  return for_statement_node;
}

NodePtr Parser::ParseIfStatement()
{
  Node::Kind const block_kind = blocks_.back();
  if (block_kind == Node::Kind::Root)
  {
    AddError("if statement not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  NodePtr if_statement_node = std::make_shared<Node>(Node(Node::Kind::IfStatement, token_));
  do
  {
    if (token_->kind == Token::Kind::If)
    {
      BlockNodePtr      if_node = std::make_shared<BlockNode>(BlockNode(Node::Kind::If, token_));
      ExpressionNodePtr expression_node = ParseConditionalExpression();
      if (expression_node == nullptr)
      {
        return nullptr;
      }
      if_node->children.push_back(std::move(expression_node));
      if (ParseBlock(*if_node) == false)
      {
        return nullptr;
      }
      if_statement_node->children.push_back(std::move(if_node));
      continue;
    }
    else if (token_->kind == Token::Kind::ElseIf)
    {
      BlockNodePtr elseif_node = std::make_shared<BlockNode>(BlockNode(Node::Kind::ElseIf, token_));
      ExpressionNodePtr expression_node = ParseConditionalExpression();
      if (expression_node == nullptr)
      {
        return nullptr;
      }
      elseif_node->children.push_back(std::move(expression_node));
      if (ParseBlock(*elseif_node) == false)
      {
        return nullptr;
      }
      if_statement_node->children.push_back(std::move(elseif_node));
      continue;
    }
    else if (token_->kind == Token::Kind::Else)
    {
      BlockNodePtr else_node = std::make_shared<BlockNode>(BlockNode(Node::Kind::Else, token_));
      if (ParseBlock(*else_node) == false)
      {
        return nullptr;
      }
      if_statement_node->children.push_back(std::move(else_node));
      continue;
    }
    return if_statement_node;
  } while (true);
}

NodePtr Parser::ParseVarStatement()
{
  Node::Kind const block_kind = blocks_.back();
  if (block_kind == Node::Kind::Root)
  {
    AddError("variable declaration not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  NodePtr var_statement_node =
      std::make_shared<Node>(Node(Node::Kind::VarDeclarationStatement, token_));
  Next();
  if (token_->kind != Token::Kind::Identifier)
  {
    AddError("expected variable name");
    return nullptr;
  }
  ExpressionNodePtr identifier_node =
      std::make_shared<ExpressionNode>(ExpressionNode(Node::Kind::Identifier, token_));
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
  if ((type == false) && (assign == false))
  {
    AddError("expected ':' or '='");
    return nullptr;
  }
  if (type == false)
  {
    var_statement_node->kind = Node::Kind::VarDeclarationTypelessAssignmentStatement;
  }
  else if (assign)
  {
    var_statement_node->kind = Node::Kind::VarDeclarationTypedAssignmentStatement;
  }
  return var_statement_node;
}

NodePtr Parser::ParseReturnStatement()
{
  Node::Kind const block_kind = blocks_.back();
  if (block_kind == Node::Kind::Root)
  {
    AddError("return statement not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  NodePtr return_statement_node = std::make_shared<Node>(Node(Node::Kind::ReturnStatement, token_));
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
  Node::Kind const block_kind = blocks_.back();
  if (block_kind == Node::Kind::Root)
  {
    AddError("break statement not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  NodePtr break_statement_node = std::make_shared<Node>(Node(Node::Kind::BreakStatement, token_));
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
  Node::Kind const block_kind = blocks_.back();
  if (block_kind == Node::Kind::Root)
  {
    AddError("continue statement not permitted at topmost scope");
    // Move one token on so GoToNextStatement() can work properly
    Next();
    return nullptr;
  }
  NodePtr continue_statement_node =
      std::make_shared<Node>(Node(Node::Kind::ContinueStatement, token_));
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
  Node::Kind const block_kind = blocks_.back();
  if (block_kind == Node::Kind::Root)
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
  Node::Kind kind;
  if (token_->kind == Token::Kind::ModuloAssign)
  {
    kind = Node::Kind::ModuloAssignOp;
  }
  else if (token_->kind == Token::Kind::AddAssign)
  {
    kind = Node::Kind::AddAssignOp;
  }
  else if (token_->kind == Token::Kind::SubtractAssign)
  {
    kind = Node::Kind::SubtractAssignOp;
  }
  else if (token_->kind == Token::Kind::MultiplyAssign)
  {
    kind = Node::Kind::MultiplyAssignOp;
  }
  else if (token_->kind == Token::Kind::DivideAssign)
  {
    kind = Node::Kind::DivideAssignOp;
  }
  else if (token_->kind == Token::Kind::Assign)
  {
    kind = Node::Kind::AssignOp;
  }
  else
  {
    AddError("expected ';' or assignment operator");
    return nullptr;
  }
  // NOTE: This doesn't handle chains of assignment a = b = c by design
  ExpressionNodePtr assignment_node =
      std::make_shared<ExpressionNode>(ExpressionNode(kind, token_));
  ExpressionNodePtr rhs = ParseExpression();
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
    if ((token_->kind == Token::Kind::AnnotationIdentifier) ||
        (token_->kind == Token::Kind::Function) || (token_->kind == Token::Kind::While) ||
        (token_->kind == Token::Kind::For) || (token_->kind == Token::Kind::If) ||
        (token_->kind == Token::Kind::Var) || (token_->kind == Token::Kind::Return) ||
        (token_->kind == Token::Kind::Break) || (token_->kind == Token::Kind::Continue))
    {
      Undo();
      return;
    }
    Next();
  }
}

void Parser::SkipFunctionDefinition()
{
  while ((token_->kind != Token::Kind::EndOfInput) && (token_->kind != Token::Kind::EndFunction))
  {
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
    AddError("expected type name");
    return nullptr;
  }
  std::string       name = token_->text;
  ExpressionNodePtr identifier_node =
      std::make_shared<ExpressionNode>(ExpressionNode(Node::Kind::Identifier, token_));
  if (IsTemplateName(name) == false)
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
      std::make_shared<ExpressionNode>(ExpressionNode(Node::Kind::Template, token_));
  template_node->children.push_back(std::move(identifier_node));
  do
  {
    ExpressionNodePtr subtype_node = ParseType();
    if (subtype_node == nullptr)
    {
      return nullptr;
    }
    name += subtype_node->token.text;
    template_node->children.push_back(std::move(subtype_node));
    Next();
    if (token_->kind == Token::Kind::Comma)
    {
      name += ", ";
      continue;
    }
    if (token_->kind == Token::Kind::GreaterThan)
    {
      name += ">";
      template_node->token.text = name;
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
    switch (token_->kind)
    {
    case Token::Kind::Identifier:
    {
      if (HandleIdentifier() == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::Integer32:
    {
      if (HandleLiteral(Node::Kind::Integer32) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::UnsignedInteger32:
    {
      if (HandleLiteral(Node::Kind::UnsignedInteger32) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::Integer64:
    {
      if (HandleLiteral(Node::Kind::Integer64) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::UnsignedInteger64:
    {
      if (HandleLiteral(Node::Kind::UnsignedInteger64) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::Float32:
    {
      if (HandleLiteral(Node::Kind::Float32) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::Float64:
    {
      if (HandleLiteral(Node::Kind::Float64) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::String:
    {
      if (HandleLiteral(Node::Kind::String) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::True:
    {
      if (HandleLiteral(Node::Kind::True) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::False:
    {
      if (HandleLiteral(Node::Kind::False) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::Null:
    {
      if (HandleLiteral(Node::Kind::Null) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::Modulo:
    {
      if (HandleBinaryOp(Node::Kind::ModuloOp, OpInfo(6, Association::Left, 2)) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::Plus:
    {
      HandlePlus();
      break;
    }
    case Token::Kind::Minus:
    {
      HandleMinus();
      break;
    }
    case Token::Kind::Multiply:
    {
      if (HandleBinaryOp(Node::Kind::MultiplyOp, OpInfo(6, Association::Left, 2)) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::Divide:
    {
      if (HandleBinaryOp(Node::Kind::DivideOp, OpInfo(6, Association::Left, 2)) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::Equal:
    {
      if (HandleBinaryOp(Node::Kind::EqualOp, OpInfo(3, Association::Left, 2)) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::NotEqual:
    {
      if (HandleBinaryOp(Node::Kind::NotEqualOp, OpInfo(3, Association::Left, 2)) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::LessThan:
    {
      if (HandleBinaryOp(Node::Kind::LessThanOp, OpInfo(4, Association::Left, 2)) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::LessThanOrEqual:
    {
      if (HandleBinaryOp(Node::Kind::LessThanOrEqualOp, OpInfo(4, Association::Left, 2)) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::GreaterThan:
    {
      if (HandleBinaryOp(Node::Kind::GreaterThanOp, OpInfo(4, Association::Left, 2)) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::GreaterThanOrEqual:
    {
      if (HandleBinaryOp(Node::Kind::GreaterThanOrEqualOp, OpInfo(4, Association::Left, 2)) ==
          false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::And:
    {
      if (HandleBinaryOp(Node::Kind::AndOp, OpInfo(2, Association::Left, 2)) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::Or:
    {
      if (HandleBinaryOp(Node::Kind::OrOp, OpInfo(1, Association::Left, 2)) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::Not:
    {
      HandleNot();
      break;
    }
    case Token::Kind::Inc:
    {
      HandleIncDec(Node::Kind::PrefixIncOp, OpInfo(7, Association::Right, 1),
                   Node::Kind::PostfixIncOp, OpInfo(8, Association::Left, 1));
      break;
    }
    case Token::Kind::Dec:
    {
      HandleIncDec(Node::Kind::PrefixDecOp, OpInfo(7, Association::Right, 1),
                   Node::Kind::PostfixDecOp, OpInfo(8, Association::Left, 1));
      break;
    }
    case Token::Kind::LeftParenthesis:
    {
      if (HandleOpener(Node::Kind::ParenthesisGroup, Node::Kind::InvokeOp,
                       Token::Kind::RightParenthesis, ")") == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::LeftSquareBracket:
    {
      if (HandleOpener(Node::Kind::Unknown, Node::Kind::IndexOp, Token::Kind::RightSquareBracket,
                       "]") == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::RightParenthesis:
    case Token::Kind::RightSquareBracket:
    {
      if (HandleCloser(is_conditional_expression) == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::Dot:
    {
      if (HandleDot() == false)
      {
        return nullptr;
      }
      break;
    }
    case Token::Kind::Comma:
    {
      if (HandleComma() == false)
      {
        return nullptr;
      }
      break;
    }
    default:
    {
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
    }
    }  // switch
  } while (found_expression_terminator_ == false);
  if (groups_.size())
  {
    Expr const &groupop = operators_[groups_.back()];
    AddError("expected '" + groupop.closer_token_text + "'");
    return nullptr;
  }
  // Roll back so token_ is pointing at the last token of the expression
  Undo();
  while (operators_.size())
  {
    Expr &topop = operators_.back();
    rpn_.push_back(std::move(topop));
    operators_.pop_back();
  }
  // rpn_ holds the Reverse Polish Notation (aka postfix) expression
  // Here we convert the RPN to an infix expression tree
  for (std::size_t i = 0; i < rpn_.size(); ++i)
  {
    Expr &expr = rpn_[i];
    if ((expr.node->kind == Node::Kind::ParenthesisGroup) ||
        (expr.node->kind == Node::Kind::UnaryPlusOp))
    {
      // Just ignore these no-ops
      continue;
    }
    if (expr.is_operator)
    {
      std::size_t const arity = std::size_t(expr.op_info.arity);
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
  return std::move(infix_stack_.front().node);
}

bool Parser::HandleIdentifier()
{
  if (state_ == State::PostOperand)
  {
    found_expression_terminator_ = true;
    return true;
  }
  std::string name;
  if (ParseExpressionIdentifier(name) == false)
  {
    return false;
  }
  state_ = State::PostOperand;
  return true;
}

bool Parser::ParseExpressionIdentifier(std::string &name)
{
  AddOperand(Node::Kind::Identifier);
  name = token_->text;
  if (IsTemplateName(name) == false)
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
  AddGroup(Node::Kind::Template, 1, Token::Kind::GreaterThan, ">");
  do
  {
    Next();
    if (token_->kind != Token::Kind::Identifier)
    {
      AddError("expected type name");
      return false;
    }
    std::string subtypename;
    if (ParseExpressionIdentifier(subtypename) == false)
    {
      return false;
    }
    name += subtypename;
    Next();
    if (token_->kind == Token::Kind::Comma)
    {
      name += ", ";
      Expr &groupop = operators_.back();
      (groupop.op_info.arity)++;
      continue;
    }
    if (token_->kind == Token::Kind::GreaterThan)
    {
      name += ">";
      Expr &groupop            = operators_.back();
      groupop.node->token.text = name;
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

bool Parser::HandleLiteral(Node::Kind kind)
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
    HandleOp(Node::Kind::UnaryPlusOp, OpInfo(7, Association::Right, 1));
  }
  else
  {
    HandleOp(Node::Kind::AddOp, OpInfo(5, Association::Left, 2));
    state_ = State::PreOperand;
  }
}

void Parser::HandleMinus()
{
  if (state_ == State::PreOperand)
  {
    HandleOp(Node::Kind::UnaryMinusOp, OpInfo(7, Association::Right, 1));
  }
  else
  {
    HandleOp(Node::Kind::SubtractOp, OpInfo(5, Association::Left, 2));
    state_ = State::PreOperand;
  }
}

bool Parser::HandleBinaryOp(Node::Kind kind, OpInfo const &op_info)
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
    HandleOp(Node::Kind::NotOp, OpInfo(7, Association::Right, 1));
  }
  else
  {
    found_expression_terminator_ = true;
  }
}

void Parser::HandleIncDec(Node::Kind prefix_kind, OpInfo const &prefix_op_info,
                          Node::Kind postfix_kind, OpInfo const &postfix_op_info)
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
  ExpressionNodePtr dot_node =
      std::make_shared<ExpressionNode>(ExpressionNode(Node::Kind::DotOp, token_));
  Next();
  if (token_->kind != Token::Kind::Identifier)
  {
    AddError("expected identifier");
    return false;
  }
  AddOperand(Node::Kind::Identifier);
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

bool Parser::HandleOpener(Node::Kind prefix_kind, Node::Kind postfix_kind,
                          Token::Kind closer_token_kind, std::string const &closer_token_text)
{
  if (state_ == State::PreOperand)
  {
    if (prefix_kind == Node::Kind::Unknown)
    {
      AddError("expected expression");
      return false;
    }
    AddGroup(prefix_kind, 0, closer_token_kind, closer_token_text);
    return true;
  }
  if (postfix_kind == Node::Kind::Unknown)
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
  while (operators_.size())
  {
    Expr &topop = operators_.back();
    if (topop.node->kind != groupop.node->kind)
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
      if ((groupop.node->kind == Node::Kind::ParenthesisGroup) ||
          (groupop.node->kind == Node::Kind::IndexOp))
      {
        AddError("expected expression");
        return false;
      }
    }
    if ((groupop.node->kind == Node::Kind::ParenthesisGroup) && (groups_.size() == 1) &&
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
  if (groupop.node->kind == Node::Kind::ParenthesisGroup)
  {
    // Commas are not allowed inside a parenthesis group
    AddError("");
    return false;
  }
  while (operators_.size())
  {
    Expr &topop = operators_.back();
    if (topop.node->kind == groupop.node->kind)
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

void Parser::HandleOp(Node::Kind kind, OpInfo const &op_info)
{
  Node::Kind group_kind;
  bool       check_if_group_opener = false;
  if (groups_.size())
  {
    Expr const &groupop   = operators_[groups_.back()];
    group_kind            = groupop.node->kind;
    check_if_group_opener = true;
  }
  while (operators_.size())
  {
    Expr &topop = operators_.back();
    if ((check_if_group_opener) && (topop.node->kind == group_kind))
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

void Parser::AddGroup(Node::Kind kind, int arity, Token::Kind closer_token_kind,
                      std::string const &closer_token_text)
{
  IncrementGroupMembers();
  Expr expr;
  expr.is_operator       = true;
  expr.node              = std::make_shared<ExpressionNode>(ExpressionNode(kind, token_));
  expr.op_info.arity     = arity;
  expr.closer_token_kind = closer_token_kind;
  expr.closer_token_text = closer_token_text;
  expr.num_members       = 0;
  groups_.push_back(operators_.size());
  operators_.push_back(std::move(expr));
}

void Parser::AddOp(Node::Kind kind, OpInfo const &op_info)
{
  IncrementGroupMembers();
  Expr expr;
  expr.is_operator       = true;
  expr.node              = std::make_shared<ExpressionNode>(ExpressionNode(kind, token_));
  expr.op_info           = op_info;
  expr.closer_token_kind = Token::Kind::Unknown;
  expr.num_members       = 0;
  operators_.push_back(std::move(expr));
}

void Parser::AddOperand(Node::Kind kind)
{
  IncrementGroupMembers();
  Expr expr;
  expr.is_operator       = false;
  expr.node              = std::make_shared<ExpressionNode>(ExpressionNode(kind, token_));
  expr.closer_token_kind = Token::Kind::Unknown;
  expr.num_members       = 0;
  rpn_.push_back(std::move(expr));
}

void Parser::AddError(std::string const &message)
{
  std::stringstream stream;
  stream << "line " << token_->line << ": ";
  if (token_->kind != Token::Kind::EndOfInput)
  {
    stream << "error at '" << token_->text << "'";
  }
  else
  {
    stream << "reached end-of-input";
  }
  if (message.length())
  {
    stream << ", " << message;
  }
  errors_.push_back(stream.str());
}

}  // namespace vm
}  // namespace fetch
