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

#include "core/byte_array/decoders.hpp"
#include "logging/logging.hpp"
#include "yaml/document.hpp"
#include "yaml/exceptions.hpp"

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <map>
#include <string>

namespace fetch {
namespace yaml {

/**
 * Extract a primitive value from a YamlToken
 *
 * @param variant The output variant
 * @param token The token to be converted
 * @param document The whole document
 */
void YamlDocument::ExtractPrimitive(Variant &variant, YamlToken const &token,
                                    ConstByteArray const &document)
{
  bool        success{false};
  char const *str = nullptr;

  ConstByteArray array = {};

  uint64_t pos      = 0;
  uint64_t prev_pos = 0;

  switch (token.type)
  {
  case KEYWORD_TRUE:
    variant = true;
    success = true;
    break;

  case KEYWORD_FALSE:
    variant = false;
    success = true;
    break;

  case KEYWORD_INF:
    variant = std::numeric_limits<double>::infinity();
    success = true;
    break;

  case KEYWORD_NEG_INF:
    variant = -std::numeric_limits<double>::infinity();
    success = true;
    break;

  case KEYWORD_NAN:
    variant = std::numeric_limits<double>::quiet_NaN();
    success = true;
    break;

  case KEYWORD_NULL:
    variant = Variant::Null();
    success = true;
    break;

  case STRING:
  case STRING_MULTILINE:
    pos = token.first;

    while (pos <= token.second)
    {
      prev_pos = pos;
      // TODO(issue 1530): There is potentially a bug here -- double check logic
      while ((pos < document.size()) && (pos <= token.second) && (document[pos] != 0x0A) &&
             (document[pos] != 0x0D) && (document[pos] != '\\'))
      {
        ++pos;
      }

      if (array.empty())
      {
        array = document.SubArray(prev_pos, pos - prev_pos);
      }
      else
      {
        array = array + document.SubArray(prev_pos, pos - prev_pos);
      }

      char const &c = *(document.char_pointer() + pos);

      if (c == 0x0A || c == 0x0D)
      {
        while ((pos <= token.second) && ((document[pos] == 0x0D) || (document[pos] == 0x0A)))
        {
          ++pos;
        }

        if (pos < token.second)
        {
          if (token.type == STRING_MULTILINE)
          {
            array = array + "\n";
          }
          else
          {
            array = array + " ";
          }
        }

        while (pos <= token.second && (document[pos] == 0x20))
        {
          ++pos;
        }

        --pos;
      }
      else if (c == '\\')
      {
        if (pos <= token.second - 1)
        {
          ++pos;
          char const &c1 = *(document.char_pointer() + pos);

          switch (c1)
          {
          case 'n':
            array = array + "\n";
            break;
          case 'r':
            array = array + "\r";
            break;
          case 't':
            array = array + "\t";
            break;
          case 'b':
            array = array + "\b";
            break;
          case 'x':
            if (pos <= token.second)
            {
              array = array + byte_array::FromHex(document.SubArray(pos + 1, 2));
            }
            pos += 2;
            break;
          case 'u':
            if (pos <= token.second)
            {
              array = array + byte_array::FromHex(document.SubArray(pos + 1, 4));
            }
            pos += 4;
            break;
          default:
            array = array + document.SubArray(pos - 1, 2);
            break;
          }
        }
      }

      ++pos;
    }

    variant = array;
    success = true;
    break;
  case NUMBER_INT:
    str     = document.char_pointer() + token.first;
    variant = std::strtoll(str, nullptr, 10);
    if (errno == ERANGE)
    {
      errno = 0;
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to convert str=", str, " to integer");
      throw YamlParseException(std::string("Failed to convert str=") + str + " to integer");
    }
    success = true;
    break;

  case NUMBER_HEX:
    if ((token.second - token.first) < 2)
    {
      throw YamlParseException("Invalid hex number length!");
    }

    str     = document.char_pointer() + token.first + 2;
    variant = std::strtoll(str, nullptr, 16);
    if (errno == ERANGE)
    {
      errno = 0;
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to convert str=", str, " to integer");
      throw YamlParseException(std::string("Failed to convert str=") + str + " to integer");
    }
    success = true;
    break;

  case NUMBER_OCT:
    if ((token.second - token.first) < 2)
    {
      throw YamlParseException("Invalid oct number length!");
    }

    str     = document.char_pointer() + token.first + 2;
    variant = std::strtoll(str, nullptr, 8);
    if (errno == ERANGE)
    {
      errno = 0;
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to convert str=", str, " to integer");
      throw YamlParseException(std::string("Failed to convert str=") + str + " to integer");
    }
    success = true;
    break;

  case NUMBER_FLOAT:
    str     = document.char_pointer() + token.first;
    variant = std::strtold(str, nullptr);
    if (errno == ERANGE)
    {
      errno = 0;
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to convert str=", str, " to long double");

      throw YamlParseException(std::string("Failed to convert str=") + str + " to long double");
    }
    success = true;
    break;

  default:
    break;
  }

  if (!success)
  {
    throw YamlParseException("Unable to parse primitive data value");
  }
}

YamlDocument::YamlObject *YamlDocument::FindInStack(std::vector<YamlObject> &stack, uint ident)
{
  while (!stack.empty())
  {
    if (stack.back().ident > ident)
    {
      stack.pop_back();
    }
    else
    {
      return &stack.back();
    }
  }

  return nullptr;
}

/**
 * Parse a Yaml document
 *
 * @param document The input document
 */
void YamlDocument::Parse(ConstByteArray const &document)
{
  using VariantStack = std::vector<YamlObject>;

  // tokenise the document
  Tokenise(document);

  enum class ObjectState
  {
    NA,
    KEY,
    VALUE,
  };

  ConstByteArray                      key;
  ConstByteArray                      alias;
  ObjectState                         state{ObjectState::NA};
  VariantStack                        variant_stack = {};
  std::map<ConstByteArray, Variant *> alias_mapping;

  // process all the token
  for (std::size_t idx = 0, end = tokens_.size(); idx < end; ++idx)
  {
    YamlToken &token = tokens_[idx];

    // determine if this is a primitive type
    bool const is_primitive = (token.type == KEYWORD_TRUE) || (token.type == KEYWORD_FALSE) ||
                              (token.type == KEYWORD_NULL) || (token.type == STRING) ||
                              (token.type == STRING_MULTILINE) || (token.type == NUMBER_INT) ||
                              (token.type == NUMBER_FLOAT) || (token.type == NUMBER_HEX) ||
                              (token.type == NUMBER_OCT) || (token.type == KEYWORD_INF) ||
                              (token.type == KEYWORD_NEG_INF) || (token.type == KEYWORD_NAN);

    if (is_primitive)
    {
      // handle the case for an object serialisation
      if (state == ObjectState::KEY)
      {
        throw YamlParseException("Invalid state");
      }

      YamlObject *current = (variant_stack.empty()) ? nullptr : &variant_stack.back();

      if (current == nullptr)
      {
        ExtractPrimitive(variant_, token, document);
        variant_stack.push_back({&variant_, token.ident, token.line});
        continue;
      }

      if (state == ObjectState::VALUE)
      {
        Variant value;
        ExtractPrimitive(value, token, document);
        (*current->data)[key] = value;

        if (idx > 0 && tokens_[idx - 1].type == ALIAS)
        {
          alias_mapping[alias] = &(*current->data)[key];
        }
        state = ObjectState::KEY;
      }
      else if (current->data->IsArray())
      {
        std::size_t const next_idx = current->data->size();
        current->data->ResizeArray(next_idx + 1);

        Variant value;
        // extract the primitive value
        ExtractPrimitive(value, token, document);
        (*current->data)[next_idx] = value;

        if (idx > 0 && tokens_[idx - 1].type == ALIAS)
        {
          alias_mapping[alias] = &(*current->data)[next_idx];
        }
      }
      else
      {
        throw YamlParseException(
            "Invalid parser state: value mode, but previous element in stack is not array and not "
            "object");
      }
    }
    else if (token.type == KEYWORD_CONTENT)
    {
      // Skipping document start
    }
    else if (token.type == TAG)
    {
      // Skipping tags
    }
    else if (token.type == ALIAS)
    {
      alias           = document.SubArray(token.first, token.second - token.first + 1);
      std::string str = static_cast<std::string>(alias);
      // Just skip
    }
    else if (token.type == ALIAS_REFERENCE)
    {
      alias = document.SubArray(token.first, token.second - token.first + 1);

      std::string str = static_cast<std::string>(alias);

      auto alias_find = alias_mapping.find(alias);
      if (alias_find == alias_mapping.end())
      {
        throw YamlParseException("Object not found by reference!");
      }

      YamlObject *current = variant_stack.empty() ? nullptr : &variant_stack.back();

      if (current == nullptr)
      {
        throw YamlParseException("Invalid parser state: reference detected but nothing in stack!");
      }
      if (current->data->IsArray())
      {
        std::size_t const next_idx = current->data->size();
        current->data->ResizeArray(next_idx + 1);
        (*current->data)[next_idx] = *alias_find->second;
      }
      else if (current->data->IsObject())
      {
        if (state == ObjectState::VALUE)
        {
          (*current->data)[key] = *alias_find->second;
          state                 = ObjectState::KEY;
        }
        else
        {
          throw YamlParseException("Cannot insert reference to object, not in value mode");
        }
      }
      else
      {
        throw YamlParseException(
            "Invalid parser state: reference detected but no object or array in stack!");
      }
    }
    else if (token.type == COMMENT)
    {
      // skipping comment
      // TODO(issue 1522): probably, add it somewhere
    }
    else if (token.type == OPEN_OBJECT)
    {
      // if this is the first element that has been seen in this document
      if (variant_stack.empty())
      {
        // define the initial object and add it to the stack
        variant_ = Variant::Object();
        variant_stack.push_back({&variant_, token.ident, token.line});

        if (idx > 0 && tokens_[idx - 1].type == ALIAS)
        {
          alias_mapping[alias] = &variant_;
        }
      }
      else
      {
        YamlObject *context = FindInStack(variant_stack, token.ident);

        if (ObjectState::VALUE == state)  // This is a child object, and we already have a key
        {
          assert(context->data->IsObject());

          Variant &next_element = (*context->data)[key];
          next_element = Variant::Object();  // Create a new object and put it to parent object

          variant_stack.push_back({&next_element, token.ident, token.line});

          if (idx > 0 && tokens_[idx - 1].type == ALIAS)
          {
            alias_mapping[alias] = &next_element;
          }
        }
        else if (context->data->IsArray())  // This is a child object inside array
        {
          std::size_t const next_idx = context->data->size();
          context->data->ResizeArray(next_idx + 1);

          // create the object inside of the array
          Variant &next_element = (*context->data)[next_idx];
          next_element          = Variant::Object();  // Create a new object and add it into array

          // add it to the stack
          variant_stack.push_back({&next_element, token.ident, token.line});

          if (idx > 0 && tokens_[idx - 1].type == ALIAS)
          {
            alias_mapping[alias] = &next_element;
          }
        }
        else
        {
          throw YamlParseException("Invalid parser state");
        }
      }

      // update our key/value state
      state = ObjectState::KEY;
    }
    else if (token.type == KEY)
    {
      if (idx > 0)
      {
        YamlToken const &prevToken = tokens_[idx - 1];
        if (prevToken.line == token.line && prevToken.type == NEW_MULTILINE_ENTRY)
        {
          token.ident = prevToken.ident + static_cast<uint>(token.first - prevToken.second);
        }
        else if (prevToken.type == OPEN_OBJECT)
        {
          YamlObject *obj = &variant_stack.back();
          obj->ident      = token.ident;
        }
      }

      YamlObject *context = FindInStack(variant_stack, token.ident);

      if (context == nullptr || (token.ident > context->ident &&
                                 token.line != context->line))  // There were no objects in stack
      {
        // define the initial object and add it to the stack
        if (variant_stack.empty())
        {
          variant_ = Variant::Object();
          variant_stack.push_back({&variant_, token.ident, token.line});
        }
        else if (context != nullptr)
        {
          if (context->data->IsArray())  // add array element
          {
            std::size_t const next_idx = context->data->size();
            context->data->ResizeArray(next_idx + 1);

            // create the object inside of the array
            Variant &next_element = (*context->data)[next_idx];
            next_element          = Variant::Object();

            variant_stack.push_back({&next_element, token.ident, token.line});
          }
          else if (context->data->IsObject())
          {
            // TODO(issue 1523): check value mode
            Variant &next_element = (*context->data)[key];
            next_element          = Variant::Object();

            variant_stack.push_back({&next_element, token.ident, token.line});
          }
          else
          {
            throw YamlParseException("Invalid parser state");
          }
        }
        else
        {
          throw YamlParseException("Invalid parser state");
        }

        context = &variant_stack.back();
        if (idx > 0 && tokens_[idx - 1].type == ALIAS)
        {
          alias_mapping[alias] = context->data;
        }
      }
      else
      {                                   // We have an object in stack
        if (ObjectState::VALUE == state)  // create a new object and add it to parent
        {
          assert(context->data->IsObject());

          if (idx > 0 && tokens_[idx - 1].type == KEY && tokens_[idx - 1].ident == token.ident)
          {
            Variant &next_element = (*context->data)[key];
            next_element          = Variant::Null();
          }
          else
          {
            Variant &next_element = (*context->data)[key];
            next_element          = Variant::Object();

            variant_stack.push_back({&next_element, token.ident, token.line});

            if (idx > 0 && tokens_[idx - 1].type == ALIAS)
            {
              alias_mapping[alias] = &next_element;
            }
          }
        }
        else if (context->data->IsArray())  // add array element
        {
          std::size_t const next_idx = context->data->size();
          context->data->ResizeArray(next_idx + 1);

          // create the object inside of the array
          Variant &next_element = (*context->data)[next_idx];
          next_element          = Variant::Object();

          uint ident = token.ident;
          if (context->line == token.line)
          {
            ident = context->ident + 2;  // compacted sequence. May be, better calculate spacing
                                         // between, but actually not necessary
          }

          // add it to the stack
          variant_stack.push_back({&next_element, ident, token.line});

          if (idx > 0 && tokens_[idx - 1].type == ALIAS)
          {
            alias_mapping[alias] = &next_element;
          }
        }
        else
        {
          assert(context->data->IsObject());
          context->ident = token.ident;  // For multiline
        }
      }

      key   = document.SubArray(token.first, token.second - token.first + 1);
      state = ObjectState::VALUE;
    }
    else if (token.type == CLOSE_OBJECT)
    {
      assert(variant_stack.back().data->IsObject());

      // drop this current object from the stack
      variant_stack.pop_back();

      YamlObject *next = (variant_stack.empty()) ? nullptr : &variant_stack.back();

      // based on the next item in the stack choose the correct object state
      if (next->data->IsObject())
      {
        state = ObjectState::KEY;
      }
      else
      {
        state = ObjectState::NA;
      }
    }
    else if (token.type == OPEN_ARRAY)
    {
      YamlObject *context = FindInStack(variant_stack, token.ident);

      if (context == nullptr)
      {
        if (variant_stack.empty())
        {
          variant_ = Variant::Array(0);
          variant_stack.push_back({&variant_, token.ident, token.line});

          if (idx > 0 && tokens_[idx - 1].type == ALIAS)
          {
            alias_mapping[alias] = &variant_;
          }
        }
        else
        {
          throw YamlParseException("Invalid parser state");
          // Variant next = Variant::Array(0);
          // variant_stack.push_back({&next, token.ident, token.line});
        }
      }
      else
      {
        if (ObjectState::VALUE == state)
        {
          assert(context->data->IsObject());

          Variant &next_element = (*context->data)[key];
          next_element          = Variant::Array(0);

          variant_stack.push_back({&next_element, token.ident, token.line});

          if (idx > 0 && tokens_[idx - 1].type == ALIAS)
          {
            alias_mapping[alias] = &next_element;
          }
        }
        else if (context->data->IsArray())
        {
          std::size_t const next_idx = context->data->size();
          context->data->ResizeArray(next_idx + 1);

          // create the object inside of the array
          Variant &next_element = (*context->data)[next_idx];
          next_element          = Variant::Array(0);

          // add it to the stack
          variant_stack.push_back({&next_element, token.ident, token.line});

          if (idx > 0 && tokens_[idx - 1].type == ALIAS)
          {
            alias_mapping[alias] = &next_element;
          }
        }
        else
        {
          throw YamlParseException("Invalid parser state");
        }
      }

      // update the object state accordingly
      state = ObjectState::NA;
    }
    else if (token.type == NEW_ENTRY || token.type == NEW_MULTILINE_ENTRY)
    {
      YamlObject *current = FindInStack(variant_stack, token.ident);

      if (current == nullptr)
      {
        if (token.type == NEW_ENTRY)
        {
          throw YamlParseException("Invalid parser state");
        }

        if (variant_stack.empty())
        {
          variant_ = Variant::Array(0);
          variant_stack.push_back({&variant_, token.ident, token.line});
        }
        else
        {  // TODO(issue 1524): either a bug, or need to get previous object and add value
          throw YamlParseException("Invalid parser state");
          // Variant next = Variant::Array(0);
          // variant_stack.push_back({&next, token.ident, token.line});
        }
      }
      else if (current->data->IsArray() && token.type == NEW_MULTILINE_ENTRY)
      {
        state = ObjectState::NA;
      }
      else if (!current->data->IsArray() && token.type == NEW_MULTILINE_ENTRY)
      {
        if (ObjectState::VALUE == state)
        {
          assert(current->data->IsObject());

          Variant &next_element = (*current->data)[key];
          next_element          = Variant::Array(0);

          variant_stack.push_back({&next_element, token.ident, token.line});
          state = ObjectState::NA;
        }
      }
      else if (current->data->IsObject() && token.type == NEW_ENTRY)
      {  // Single-line mapping, like: {key:value, key:value}
        state = ObjectState::KEY;
      }
      else if (!current->data->IsArray())
      {
        throw YamlParseException("Invalid parser state");
      }
    }
    else if (token.type == CLOSE_ARRAY)
    {
      assert(variant_stack.back().data->IsArray());
      variant_stack.pop_back();

      YamlObject *next = (variant_stack.empty()) ? nullptr : &variant_stack.back();

      // based on the next item in the stack choose the correct object state
      if ((next != nullptr) && next->data->IsObject())
      {
        state = ObjectState::KEY;
      }
      else
      {
        state = ObjectState::NA;
      }
    }
    else
    {
      throw YamlParseException("Invalid parser state");
    }
  }
}

/**
 * Tokenise the input document to be parsed
 *
 * @param document The document to tokenise
 */
void YamlDocument::Tokenise(ConstByteArray const &document)
{
  uint     ident = 0;
  uint     line  = 0;
  uint64_t pos   = 0;

  objects_ = 0;

  brace_stack_.reserve(32);
  brace_stack_.clear();

  counters_.reserve(32);
  counters_.clear();
  tokens_.reserve(1024);
  tokens_.clear();

  uint16_t element_counter = 0;

  auto const *ptr = reinterpret_cast<char const *>(document.pointer());

  while (pos < document.size())
  {
    auto        words16 = reinterpret_cast<uint16_t const *>(ptr + pos);
    auto        words   = reinterpret_cast<uint32_t const *>(ptr + pos);
    char const &c       = *(ptr + pos);

    switch (c)
    {
    case '\n':
    case '\r':
      ++line;
      ++pos;
      ident = 0;
      continue;

    case '\t':
      ident += 4;
      ++pos;
      continue;

    case ' ':
      ++ident;
      ++pos;
      continue;
    }

    // Handling keywords
    if ((document.size() - pos) >= 4)
    {
      switch (words[0])
      {
      case 0x0D2D2D2D:  // ---
      case 0x0A2D2D2D:
        ++line;
        ident = 0;
        ++objects_;
        tokens_.push_back({uint32_t(pos), pos + 3, KEYWORD_CONTENT, ident, line});
        pos += 4;
        ++element_counter;
        continue;

      case 0x202D2D2D:
      case 0x092D2D2D:
        ++objects_;
        tokens_.push_back({uint32_t(pos), pos + 3, KEYWORD_CONTENT, ident, line});
        pos += 4;
        ident += 4;
        ++element_counter;
        continue;

      case 0x65757274:  // true
        ++objects_;
        tokens_.push_back({uint32_t(pos), pos + 4, KEYWORD_TRUE, ident, line});
        ident += 4;
        pos += 4;
        ++element_counter;
        continue;

      case 0x736C6166:  // fals
        ++objects_;
        tokens_.push_back({pos, pos + 5, KEYWORD_FALSE, ident, line});
        ident += 4;
        pos += 4;

        if ((document.size() <= pos) || (document[pos] != 'e'))
        {
          throw YamlParseException(
              "Unrecognised token. Expected false, but last letter did not match.");
        }
        ++pos;
        ++ident;
        ++element_counter;
        continue;

      case 0x666E692E:  // .inf
        ++objects_;
        tokens_.push_back({pos, pos + 4, KEYWORD_INF, ident, line});
        pos += 4;
        ident += 4;
        ++element_counter;
        continue;

      case 0x4E614E2E:  // .NaN
        ++objects_;
        tokens_.push_back({pos, pos + 4, KEYWORD_NAN, ident, line});
        pos += 4;
        ident += 4;
        ++element_counter;
        continue;

      case 0x6E692E2D:  // -.in
        ++objects_;
        tokens_.push_back({pos, pos + 5, KEYWORD_NEG_INF, ident, line});
        pos += 4;
        ident += 4;

        if ((document.size() <= pos) || (document[pos] != 'f'))
        {
          throw YamlParseException(
              "Unrecognised token. Expected -.inf, but last letter did not match.");
        }
        ++pos;
        ++ident;
        ++element_counter;
        continue;
      }
    }

    uint64_t fixed_start = pos;
    bool     folded      = false;
    uint64_t oldpos      = pos;
    uint     prevLine    = 0;

    if ((document.size() - pos) >= 2)
    {
      uint brace_count = 0;

      switch (words16[0])
      {
      case 0x0D2D:  // "- "
      case 0x0A2D:  // "- "
        tokens_.push_back({pos, pos, NEW_MULTILINE_ENTRY, ident, line});
        pos += 2;
        ++line;
        ident = 0;
        continue;
      case 0x092D:  // "- "
      case 0x202D:  // "- "
        tokens_.push_back({pos, pos, NEW_MULTILINE_ENTRY, ident, line});
        pos += 2;
        ident += 2;
        continue;
      case 0x3C21:  //!<
        brace_count = 1;
        pos += 2;
        tokens_.push_back({oldpos, pos, TAG, ident, line});
        ident += 2;

        while (pos < document.size() && brace_count > 0)
        {
          ++ident;

          if (*(ptr + pos) == '<')
          {
            ++brace_count;
          }
          else if (*(ptr + pos) == '>')
          {
            --brace_count;
          }
          else if (*(ptr + pos) == '\r' || *(ptr + pos) == '\n')
          {
            ++line;
            ident = 0;
          }

          ++pos;
        }
        tokens_.back().second = pos;

        continue;
      case 0x0D3A:  //:
      case 0x0A3A:  //:
        tokens_.back().type = KEY;
        pos += 2;
        ++line;
        ident = 0;
        continue;
      case 0x093A:  //:
      case 0x203A:  //:
        tokens_.back().type = KEY;
        pos += 2;
        ident += 2;
        continue;
      }
    }

    switch (c)
    {
    case '#':
      byte_array::consumers::LineConsumer<COMMENT>(document, pos);
      tokens_.push_back({oldpos, pos, COMMENT, ident, line});
      ident = 0;
      break;

    case '\'':
    case '"':
      ++objects_;
      ++element_counter;
      ++pos;
      tokens_.push_back({oldpos + 1, pos, STRING, ident, line});

      while (pos < document.size() - 1)
      {
        char const &c1 = *(ptr + pos);
        char const &c2 = *(ptr + pos + 1);
        if (c == '\'' && c1 == '\'' && c2 == '\'')
        {
          pos += 2;
        }
        else if (c1 == 0x0a || c1 == 0x0d)
        {
          ident = 0;
          ++pos;
        }
        else if (c1 == c)
        {
          break;
        }
        else
        {
          ++pos;
          ++ident;
        }
      }
      tokens_.back().second = pos - 1;
      ++pos;
      ++ident;
      break;

    case '{':
      brace_stack_.push_back('}');
      counters_.emplace_back(element_counter);
      element_counter = 0;
      tokens_.push_back({pos, pos, OPEN_OBJECT, ident, line});
      ++pos;
      ++ident;
      break;

    case '}':
      if (brace_stack_.empty() || brace_stack_.back() != '}')
      {
        throw YamlParseException("Expected '}', but found ']'");
      }
      brace_stack_.pop_back();
      tokens_.push_back({pos, uint64_t(element_counter), CLOSE_OBJECT, ident, line});

      element_counter = counters_.back();
      counters_.pop_back();
      ++element_counter;
      ++pos;
      ++ident;
      ++objects_;
      break;

    case '[':
      brace_stack_.push_back(']');
      counters_.emplace_back(element_counter);
      element_counter = 0;
      tokens_.push_back({pos, 0, OPEN_ARRAY, ident, line});
      ++pos;
      ++ident;
      break;

    case ']':
      if (brace_stack_.empty() || brace_stack_.back() != ']')
      {
        throw YamlParseException("Expected ']', but found '}'.");
      }
      brace_stack_.pop_back();
      tokens_.push_back({pos, element_counter, CLOSE_ARRAY, ident, line});

      element_counter = counters_.back();
      ++element_counter;
      counters_.pop_back();

      ++pos;
      ++ident;
      ++objects_;
      break;

    case ',':
      tokens_.push_back({pos, pos, NEW_ENTRY, ident, line});
      ++pos;
      ++ident;
      break;

    case '!':
      ++objects_;
      ++element_counter;
      ++pos;
      byte_array::consumers::Token<TAG>(document, pos);
      tokens_.push_back({oldpos + 1, pos - 1, TAG, ident, line});
      ++ident;
      break;

    case '&':
      ++objects_;
      ++element_counter;
      ++pos;
      byte_array::consumers::Token<ALIAS>(document, pos);
      tokens_.push_back({oldpos + 1, pos - 1, ALIAS, ident, line});
      ++ident;
      break;

    case '*':
      ++objects_;
      ++element_counter;
      ++pos;
      byte_array::consumers::Token<ALIAS_REFERENCE>(document, pos);
      tokens_.push_back({oldpos + 1, pos - 1, ALIAS_REFERENCE, ident, line});
      ++ident;
      break;

    case '>':
    case '|':
      folded = c == '>';

      ++objects_;
      ++element_counter;
      ++pos;
      ++ident;

      while (pos < document.size())  // detecting initial identation  && ((*(ptr + pos)==0x10) ||
                                     // (*(ptr + pos)==0x13) || (*(ptr + pos)==0x20))
      {
        char const &cc = *(ptr + pos);

        if (cc == 0x0a || cc == 0x0d)
        {
          ident = 0;
        }
        else if (cc == 0x20)
        {
          ++ident;
        }
        else
        {
          fixed_start = pos;
          break;
        }

        ++pos;
      }

      tokens_.push_back({fixed_start, pos, static_cast<uint8_t>(folded ? STRING : STRING_MULTILINE),
                         ident, line});

      prevLine = line;
      while (pos < document.size())
      {
        char const &cc    = *(ptr + pos);
        char const &prevC = *(ptr + pos - 1);
        words16           = reinterpret_cast<uint16_t const *>(ptr + pos);

        if (words16[0] == 0x0a0d)
        {
          ident = 0;
          if (prevC == 0x0a || prevC == 0x0d)
          {
            pos -= 2;
            break;
          }
          ++line;
          pos += 2;
        }
        else if (cc == 0x0a || cc == 0x0d)
        {
          ident = 0;

          if (prevC == 0x0a || prevC == 0x0d)
          {
            pos -= 1;
            break;
          }
          ++line;
          ++pos;
        }
        else if (cc == 0x20)
        {
          ++ident;
          ++pos;
        }
        else
        {
          if ((prevLine != line) && (ident != tokens_.back().ident))
          {
            --pos;
            if (ident == 0)
            {
              --line;
            }
            break;
          }

          ++pos;
          ++ident;

          prevLine = line;
        }
      }

      tokens_.back().second = pos;
      ++pos;
      break;

    default:  // If none of the above then it is a key
      Type lastType  = COMMENT;
      uint lastLine  = 0;
      uint lastIdent = 0;

      if (!tokens_.empty())
      {
        lastType  = static_cast<Type>(tokens_.back().type);
        lastLine  = tokens_.back().line;
        lastIdent = tokens_.back().ident;
      }

      if (!tokens_.empty() && (((lastType == NUMBER_INT || lastType == NUMBER_FLOAT ||
                                 lastType == NUMBER_HEX || lastType == NUMBER_OCT) &&
                                lastLine == line) ||
                               (lastType == STRING && (lastLine == line || lastIdent == ident))))
      {
        tokens_.back().second = pos;
        tokens_.back().line   = line;

        if (lastType == NUMBER_HEX)
        {
          if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
          {
            tokens_.back().type = STRING;
          }
        }
        else if (lastType == NUMBER_INT)
        {
          if (c == 'e' || c == '.')
          {
            tokens_.back().type = NUMBER_FLOAT;
          }
          else if ((c == 'x' || c == 'X') && ((pos - tokens_.back().first) == 1))
          {
            tokens_.back().type = NUMBER_HEX;
          }
          else if ((c == 'o' || c == 'O') && ((pos - tokens_.back().first) == 1))
          {
            tokens_.back().type = NUMBER_OCT;
          }
          else if (c < '0' || c > '9')
          {
            tokens_.back().type = STRING;
          }
        }
        else if (lastType == NUMBER_FLOAT)
        {
          if (words16[0] == 0x2B65 || words16[0] == 0x2D65)
          {  // e+,e-
            tokens_.back().second = ++pos;
            // TODO(issue 1525): check it's not repeated
          }
          else if (c < '0' || c > '9')
          {
            tokens_.back().type = STRING;
          }
        }
      }
      else
      {
        if (c == 0x40 || c == 0x60)
        {
          throw YamlParseException("Reserved inidicator's can't start a plain scalar!");
        }
        ++objects_;

        if (words16[0] == 0x3078)
        {  // 0x - hex
          ++pos;
          ++ident;
          tokens_.push_back({pos + 1, pos + 1, NUMBER_HEX, ident, line});
        }
        else if ((c >= '0' && c <= '9') || c == '-' || c == '+')
        {
          // byte_array::consumers::NumberConsumer<NUMBER_FLOAT>(document, pos);
          tokens_.push_back({oldpos, pos, NUMBER_INT, ident, line});
        }
        else
        {
          // byte_array::consumers::StringConsumer<NUMBER_INT>(document, pos);
          tokens_.push_back({oldpos, pos, STRING, ident, line});
        }
      }
      ++ident;
      ++pos;

      break;
    }
  }

  if (!brace_stack_.empty())
  {
    throw YamlParseException("Object or array indicators are unbalanced.");
  }
}

}  // namespace yaml
}  // namespace fetch
