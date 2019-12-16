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

#include "semanticsearch/query/query_compiler.hpp"
#include "semanticsearch/semantic_search_module.hpp"

#include <cassert>

namespace fetch {
namespace semanticsearch {

enum
{
  OP_ADD = 0,
  OP_SUB,
  OP_ASSIGN,
  OP_MULTIPLY,
  OP_EQUAL,
  OP_SUBSCOPE,
  OP_ATTRIBUTE,
  OP_VAR_DEFINITION,
  OP_SEPARATOR,

  // Then groups
  SCOPE_OPEN = 200,
  SCOPE_CLOSE,
  PARANTHESIS_OPEN,
  PARANTHESIS_CLOSE,

  // Types
  STRING = 400,
  INTEGER,
  FLOAT,
  IDENTIFIER,

  // And finally syntax
  KEYWORD = 500,

  USER_DEFINED = 1000
};

enum
{
  MODEL,
  INSERT,
  FIND
};

QueryCompiler::QueryCompiler(ErrorTracker &                                error_tracker,
                             QueryCompiler::SemanticSearchModulePtr const &module)
  : error_tracker_(error_tracker)
  , module_{module}
{}

Query QueryCompiler::operator()(ByteArray doc, ConstByteArray const &filename)
{
  // Preparing
  document_ = std::move(doc);
  statements_.clear();
  position_   = 0;
  char_index_ = 0;
  line_       = 0;
  error_tracker_.SetSource(document_, filename);

  // Tokenizing
  Tokenise();

  // Assembling
  Query ret;
  ret.source   = document_;
  ret.filename = filename;
  for (auto &s : statements_)
  {
    auto stmt = AssembleStatement(s);
    ret.statements.push_back(stmt);
  }

  return ret;
}

std::vector<QueryInstruction> QueryCompiler::AssembleStatement(Statement const &stmt)
{
  std::vector<QueryInstruction> main_stack;
  std::vector<QueryInstruction> op_stack;

  bool last_was_identifer       = false;
  bool next_last_was_identifier = false;
  for (auto &token : stmt.tokens)
  {
    QueryInstruction next;
    next.token = token;

    last_was_identifer       = next_last_was_identifier;
    next_last_was_identifier = false;

    switch (token.type())
    {
    case OP_ADD:
      next.type       = Constants::ADD;
      next.properties = Properties::PROP_IS_OPERATOR;
      break;
    case OP_SUB:
      next.type       = Constants::SUB;
      next.properties = Properties::PROP_IS_OPERATOR;
      break;
    case OP_ASSIGN:
      next.type       = Constants::ASSIGN;
      next.properties = Properties::PROP_IS_OPERATOR;
      break;
    case OP_MULTIPLY:
      next.type       = Constants::MULTIPLY;
      next.properties = Properties::PROP_IS_OPERATOR;
      break;
    case OP_EQUAL:
      next.type       = Constants::EQUAL;
      next.properties = Properties::PROP_IS_OPERATOR;
      break;
    case OP_SUBSCOPE:
      next.type       = Constants::SUBSCOPE;
      next.properties = Properties::PROP_IS_OPERATOR;
      break;
    case OP_ATTRIBUTE:
      if (main_stack.empty())
      {
        error_tracker_.RaiseSyntaxError(
            "Expected identifier before attribute indicator, but found nothing.", token);
        return {};
      }

      if (main_stack.back().type != Constants::IDENTIFIER)
      {
        error_tracker_.RaiseSyntaxError(
            "Expected identifier before attribute indicator, but found different token.", token);
        return {};
      }

      // TODO(tfr): Chek that back is identifier or throw
      main_stack.back().type = Constants::OBJECT_KEY;
      next.type              = Constants::ATTRIBUTE;
      next.properties        = Properties::PROP_IS_OPERATOR;
      break;
    case OP_VAR_DEFINITION:
      next.type       = Constants::VAR_TYPE;
      next.properties = Properties::PROP_IS_OPERATOR;
      break;
    case OP_SEPARATOR:
      next.type       = Constants::SEPARATOR;
      next.properties = Properties::PROP_IS_OPERATOR;
      break;
    case SCOPE_OPEN:
      next.type       = Constants::PUSH_SCOPE;
      next.properties = Properties::PROP_IS_GROUP | Properties::PROP_IS_GROUP_OPEN;
      main_stack.push_back(next);
      break;
    case SCOPE_CLOSE:
      next.type       = Constants::POP_SCOPE;
      next.properties = Properties::PROP_IS_GROUP;
      break;
    case PARANTHESIS_OPEN:
      next.type       = Constants::INTERNAL_OPEN_GROUP;
      next.properties = Properties::PROP_IS_GROUP | Properties::PROP_IS_GROUP_OPEN;

      if (last_was_identifer)
      {
        next.properties |= Properties::PROP_IS_CALL;
        main_stack.back().type = Constants::FUNCTION;
      }
      break;
    case PARANTHESIS_CLOSE:
      next.type       = Constants::INTERNAL_CLOSE_GROUP;
      next.properties = Properties::PROP_IS_GROUP;
      break;

    case IDENTIFIER:
      next.type                = Constants::IDENTIFIER;
      next_last_was_identifier = true;
      next.consumes            = 0;
      break;
    case KEYWORD:
      next.type     = Constants::SET_CONTEXT;
      next.consumes = 0;
      // TODO: Use mapping held in module
      if (token == "model")
      {
        next.properties = Properties::PROP_CTX_MODEL;
      }
      else if (token == "advertise")
      {
        next.properties = Properties::PROP_CTX_ADVERTISE;
      }
      else if (token == "let")
      {
        next.properties = Properties::PROP_CTX_SET;
      }
      else if (token == "find")
      {
        next.properties = Properties::PROP_CTX_FIND;
      }
      break;

    default:
      if (token.type() < Constants::USER_DEFINED_START)
      {
        std::cerr << "TODO: Emit error" << std::endl;
        break;
      }
      next.type     = Constants::LITERAL;
      next.consumes = 0;
      break;
    }

    if ((next.properties & Properties::PROP_IS_OPERATOR) != 0)
    {
      while ((!op_stack.empty()) && (next.type > op_stack.back().type) &&
             static_cast<bool>(op_stack.back().properties & Properties::PROP_IS_OPERATOR))
      {
        auto back = op_stack.back();
        op_stack.pop_back();
        main_stack.push_back(back);
      }

      op_stack.push_back(next);
    }
    else if ((next.properties & Properties::PROP_IS_GROUP) != 0)
    {
      if ((next.properties & Properties::PROP_IS_GROUP_OPEN) != 0)
      {
        op_stack.push_back(next);
      }
      else
      {
        while ((!op_stack.empty()) &&
               ((op_stack.back().properties & Properties::PROP_IS_GROUP) == 0))
        {
          auto back = op_stack.back();
          op_stack.pop_back();
          main_stack.push_back(back);
        }

        auto b = op_stack.back();
        if ((b.properties & Properties::PROP_IS_CALL) != 0)
        {
          next.type = Constants::EXECUTE_CALL;
          main_stack.push_back(next);
        }
        else if (b.type == Constants::PUSH_SCOPE)
        {
          main_stack.push_back(next);
        }
        op_stack.pop_back();
      }
    }
    else
    {
      main_stack.push_back(next);
    }
  }

  // Emtpying the operator stack.
  while (!op_stack.empty())
  {
    auto back = op_stack.back();
    op_stack.pop_back();
    main_stack.push_back(back);
  }

  return main_stack;
}

void QueryCompiler::Tokenise()
{
  Statement stmt;
  //    std::vector< Token > tokens;
  int scope_depth = 0;
  while (position_ < document_.size())
  {
    SkipWhitespaces();
    if (position_ == document_.size())
    {
      break;
    }

    // Comments
    if (Match("//"))
    {
      SkipUntilEOL();
      continue;
    }

    if (Match("/*"))
    {
      SkipChar();
      SkipChar();

      while (!Match("*/"))
      {
        SkipChar();
      }

      SkipChar();
      SkipChar();
      continue;
    }

    // Keywords
    bool found = false;
    for (auto const &k : keywords_)
    {
      if (Match(k))
      {
        Token tok = static_cast<Token>(k);
        tok.SetLine(line_);
        tok.SetChar(char_index_);
        tok.SetType(KEYWORD);
        found = true;

        stmt.tokens.push_back(tok);
        SkipChars(k.size());
        break;
      }
    }

    if (found)
    {
      continue;
    }

    // Matching operators
    Token tok = static_cast<Token>(document_.SubArray(position_, 1));
    tok.SetLine(line_);
    tok.SetChar(char_index_);

    switch (document_[position_])
    {
    case ',':
      tok.SetType(OP_SEPARATOR);
      stmt.tokens.push_back(tok);
      SkipChar();
      continue;
    case '.':
      tok.SetType(OP_SUBSCOPE);
      stmt.tokens.push_back(tok);
      SkipChar();
      continue;
    case ':':
      if (scope_depth == 0)
      {
        tok.SetType(OP_VAR_DEFINITION);
      }
      else
      {
        tok.SetType(OP_ATTRIBUTE);
      }
      stmt.tokens.push_back(tok);
      SkipChar();
      continue;
    case '{':
      ++scope_depth;
      tok.SetType(SCOPE_OPEN);
      stmt.tokens.push_back(tok);
      SkipChar();
      continue;
    case '}':
      --scope_depth;
      tok.SetType(SCOPE_CLOSE);
      stmt.tokens.push_back(tok);
      SkipChar();
      continue;
    case '(':
      tok.SetType(PARANTHESIS_OPEN);
      stmt.tokens.push_back(tok);
      SkipChar();
      continue;
    case ')':
      tok.SetType(PARANTHESIS_CLOSE);
      stmt.tokens.push_back(tok);
      SkipChar();
      continue;
    case '+':
      tok.SetType(OP_ADD);
      stmt.tokens.push_back(tok);
      SkipChar();
      continue;
    case '*':
      tok.SetType(OP_MULTIPLY);
      stmt.tokens.push_back(tok);
      SkipChar();
      continue;
    case '=':
      if (document_[position_ + 1] == '=')
      {
        tok = static_cast<Token>(document_.SubArray(position_, 2));
        tok.SetType(OP_EQUAL);
        stmt.tokens.push_back(tok);
        SkipChars(2);
        continue;
      }
      else
      {
        tok.SetType(OP_ASSIGN);
        stmt.tokens.push_back(tok);
        SkipChar();
        continue;
      }
    case ';':
      statements_.push_back(stmt);
      stmt = Statement();
      SkipChar();
      continue;
    }

    // Searching for user defined types.
    auto const &type_info = module_->type_information();
    found                 = false;
    uint64_t end          = position_;
    for (auto const &type : type_info)
    {
      if (type.consumer)
      {
        int ret = type.consumer(document_, end);
        if (ret != -1)
        {
          tok = static_cast<Token>(document_.SubArray(position_, end - position_));
          tok.SetLine(line_);
          tok.SetChar(char_index_);
          tok.SetType(type.code);
          stmt.tokens.push_back(tok);
          SkipChars(end - position_);
          found = true;
          break;
        }
      }
    }

    if (found)
    {
      continue;
    }

    // Finding identifiers
    int type = byte_array::consumers::Token<IDENTIFIER>(document_, end);
    if (type != -1)
    {
      tok = static_cast<Token>(document_.SubArray(position_, end - position_));
      tok.SetLine(line_);
      tok.SetChar(char_index_);
      tok.SetType(type);
      stmt.tokens.push_back(tok);
      SkipChars(end - position_);
      continue;
    }

    // Error unrecognised symbol
    if (position_ < document_.size())
    {
      std::cout << "Unrecognised symbol: '" << document_[position_] << "'" << std::endl;
      std::cout << "Position: " << line_ << ":" << char_index_ << std::endl;
      exit(-1);  // TODO: Throw
    }
  }
}

bool QueryCompiler::Match(ConstByteArray const &token) const
{
  return document_.Match(token, position_);
}

void QueryCompiler::SkipUntilEOL()
{
  SkipUntil('\n');
}

void QueryCompiler::SkipWhitespaces()
{
  while ((position_ < document_.size()) &&
         (document_[position_] == '\n' || document_[position_] == '\t' ||
          document_[position_] == '\r' || document_[position_] == ' '))
  {
    SkipChar();
  }
}

void QueryCompiler::SkipChar()
{
  ++char_index_;

  if (document_[position_++] == '\n')
  {
    char_index_ = 0;
    ++line_;
  }
}

void QueryCompiler::SkipChars(uint64_t const &length)
{
  for (std::size_t i = 0; i < length; ++i)
  {
    SkipChar();
  }
}

void QueryCompiler::SkipUntil(uint8_t byte)
{
  while ((position_ < document_.size()) && (byte != document_[position_]))
  {
    SkipChar();
  }
}

}  // namespace semanticsearch
}  // namespace fetch
