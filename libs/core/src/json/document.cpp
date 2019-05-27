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

#include "core/json/document.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace fetch {
namespace json {

/**
 * Extract a primitive value from a JSONToken
 *
 * @param variant The output variant
 * @param token The token to be converted
 * @param document The whole document
 */
void JSONDocument::ExtractPrimitive(Variant &variant, JSONToken const &token,
                                    ConstByteArray const &document)
{
  bool        success{false};
  char const* str = nullptr;

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

  case KEYWORD_NULL:
    variant = Variant::Null();
    success = true;
    break;

  case STRING:
    variant = document.SubArray(token.first, token.second - token.first);
    success = true;
    break;

  case NUMBER_INT:
    str     = document.char_pointer() + token.first;
    variant = std::strtoll(str, nullptr, 10);
    if (errno == ERANGE)
    {
      errno = 0;
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to convert str=", str, " to integer");

      throw JSONParseException(std::string("Failed to convert str=") + str + " to integer");
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

      throw JSONParseException(std::string("Failed to convert str=") + str + " to long double");
    }
    success = true;
    break;

  default:
    break;
  }

  if (!success)
  {
    throw JSONParseException("Unable to parse primitive data value");
  }
}

/**
 * Parse a JSON document
 *
 * @param document The input document
 */
void JSONDocument::Parse(ConstByteArray const &document)
{
  using VariantStack = std::vector<Variant *>;

  // tokenise the document
  Tokenise(document);

  enum class ObjectState
  {
    NA,
    KEY,
    VALUE,
  };

  ConstByteArray key;
  ObjectState    state{ObjectState::NA};
  VariantStack   variant_stack = {};

  // process all the token
  for (std::size_t idx = 0, end = tokens_.size(); idx < end; ++idx)
  {
    JSONToken const &token = tokens_[idx];

    // determine if this is a primitive type
    bool const is_primitive = (token.type == KEYWORD_TRUE) || (token.type == KEYWORD_FALSE) ||
                              (token.type == KEYWORD_NULL) || (token.type == STRING) ||
                              (token.type == NUMBER_INT) || (token.type == NUMBER_FLOAT);

    if (is_primitive)
    {
      // handle the case for an object serialisation
      if (state == ObjectState::KEY)
      {
        if (token.type != STRING)
        {
          throw JSONParseException("Object key is not a string");
        }

        // since the token is a key
        key   = document.SubArray(token.first, token.second - token.first);
        state = ObjectState::VALUE;

        continue;
      }

      Variant *current = (variant_stack.empty()) ? nullptr : variant_stack.back();

      if (current == nullptr)
      {
        throw JSONParseException("Expecting a list or object as initial element");
      }

      if (state == ObjectState::VALUE)
      {
        ExtractPrimitive((*current)[key], token, document);
        state = ObjectState::KEY;
      }
      else if (current->IsArray())
      {
        std::size_t const next_idx = current->size();
        current->ResizeArray(next_idx + 1);

        // extract the primitive value
        ExtractPrimitive((*current)[next_idx], token, document);
      }
      else
      {
        assert(false);
      }
    }
    else if (token.type == OPEN_OBJECT)
    {
      // if this is the first element that has been seen in this document
      if (variant_stack.empty())
      {
        // define the initial object and add it to the stack
        variant_ = Variant::Object();
        variant_stack.push_back(&variant_);
      }
      else
      {
        Variant *context = (variant_stack.empty()) ? nullptr : variant_stack.back();

        if (ObjectState::VALUE == state)
        {
          assert(context->IsObject());

          Variant &next_element = (*context)[key];
          next_element          = Variant::Object();

          variant_stack.push_back(&next_element);
        }
        else if (context->IsArray())
        {
          std::size_t const next_idx = context->size();
          context->ResizeArray(next_idx + 1);

          // create the object inside of the array
          Variant &next_element = (*context)[next_idx];
          next_element          = Variant::Object();

          // add it to the stack
          variant_stack.push_back(&next_element);
        }
        else
        {
          throw JSONParseException("Invalid parser state");
        }
      }

      // update our key/value state
      state = ObjectState::KEY;
    }
    else if (token.type == CLOSE_OBJECT)
    {
      assert(variant_stack.back()->IsObject());

      // drop this current object from the stack
      variant_stack.pop_back();

      Variant *next = (variant_stack.empty()) ? nullptr : variant_stack.back();

      // based on the next item in the stack choose the correct object state
      if ((next != nullptr) && (next->IsObject()))
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
      if (variant_stack.empty())
      {
        variant_ = Variant::Array(0);
        variant_stack.push_back(&variant_);
      }
      else
      {
        Variant *context = (variant_stack.empty()) ? nullptr : variant_stack.back();

        if (ObjectState::VALUE == state)
        {
          assert(context->IsObject());

          Variant &next_element = (*context)[key];
          next_element          = Variant::Array(0);

          variant_stack.push_back(&next_element);
        }
        else if (context->IsArray())
        {
          std::size_t const next_idx = context->size();
          context->ResizeArray(next_idx + 1);

          // create the object inside of the array
          Variant &next_element = (*context)[next_idx];
          next_element          = Variant::Array(0);

          // add it to the stack
          variant_stack.push_back(&next_element);
        }
        else
        {
          throw JSONParseException("Invalid parser state");
        }
      }

      // update the object state accordingly
      state = ObjectState::NA;
    }
    else if (token.type == CLOSE_ARRAY)
    {
      assert(variant_stack.back()->IsArray());
      variant_stack.pop_back();

      Variant *next = (variant_stack.empty()) ? nullptr : variant_stack.back();

      // based on the next item in the stack choose the correct object state
      if ((next != nullptr) && (next->IsObject()))
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
      throw JSONParseException("Invalid parser state");
    }
  }
}

/**
 * Tokenise the input document to be parsed
 *
 * @param document The document to tokenise
 */
void JSONDocument::Tokenise(ConstByteArray const &document)
{
  int      line = 0;
  uint64_t pos  = 0;

  objects_ = 0;

  brace_stack_.reserve(32);
  brace_stack_.clear();

  counters_.reserve(32);
  counters_.clear();
  tokens_.reserve(1024);
  tokens_.clear();

  uint16_t element_counter = 0;

  char const *ptr = reinterpret_cast<char const *>(document.pointer());
  while (pos < document.size())
  {
    uint16_t const *words16 = reinterpret_cast<uint16_t const *>(ptr + pos);
    uint32_t const *words   = reinterpret_cast<uint32_t const *>(ptr + pos);
    char const &    c       = *(ptr + pos);
    if ((document.size() - pos) > 2)
    {
      // Handling white spaces
      switch (words16[0])
      {
      case 0x2020:  // a
      case 0x200A:  //
      case 0x200D:  //
      case 0x2009:  //
      case 0x0A20:  //
      case 0x0A0A:  //
      case 0x0A0D:  //
      case 0x0A09:  //
      case 0x0D20:  //
      case 0x0D0A:  //
      case 0x0D0D:  //
      case 0x0D09:  //
      case 0x0920:  //
      case 0x090A:  //
      case 0x090D:  //
      case 0x0909:  //
        pos += 2;
        continue;
      }
    }

    switch (c)
    {
    case '\n':
      ++line;
      // Falls through.
    case '\t':
    case ' ':
    case '\r':
      ++pos;
      continue;
    }
    // Handling keywords
    if ((document.size() - pos) > 4)
    {
      switch (words[0])
      {
      case 0x65757274:  // true
        ++objects_;
        tokens_.push_back({uint32_t(pos), pos + 4, KEYWORD_TRUE});
        pos += 4;
        ++element_counter;
        continue;
      case 0x736C6166:  // fals
        ++objects_;
        tokens_.push_back({pos, pos + 5, KEYWORD_FALSE});
        pos += 4;
        if ((document.size() <= pos) || (document[pos] != 'e'))
        {
          throw JSONParseException(
              "Unrecognised token. Expected false, but last letter did not match.");
        }
        ++pos;
        ++element_counter;
        continue;
      case 0x6C6C756E:  // null
        ++objects_;     // TODO(issue 35): Move
        tokens_.push_back({pos, pos + 4, KEYWORD_NULL});
        pos += 4;
        ++element_counter;
        continue;
      }
    }
    uint64_t oldpos = pos;
    uint8_t  type;

    switch (c)
    {
    case '"':
      ++objects_;
      ++element_counter;
      byte_array::consumers::StringConsumer<STRING>(document, pos);
      tokens_.push_back({oldpos + 1, pos - 1, STRING});
      break;
    case '{':
      brace_stack_.push_back('}');
      counters_.emplace_back(element_counter);
      element_counter = 0;
      tokens_.push_back({pos, 0, OPEN_OBJECT});

      ++pos;
      break;
    case '}':
      if (brace_stack_.empty() || brace_stack_.back() != '}')
      {
        throw JSONParseException("Expected '}', but found ']'");
      }
      brace_stack_.pop_back();
      tokens_.push_back({pos, uint64_t(element_counter), CLOSE_OBJECT});

      element_counter = counters_.back();
      counters_.pop_back();
      ++element_counter;
      ++pos;
      ++objects_;
      break;
    case '[':
      brace_stack_.push_back(']');
      counters_.emplace_back(element_counter);

      element_counter = 0;
      tokens_.push_back({pos, 0, OPEN_ARRAY});

      ++pos;
      break;
    case ']':
      if (brace_stack_.empty() || brace_stack_.back() != ']')
      {
        throw JSONParseException("Expected ']', but found '}'.");
      }
      brace_stack_.pop_back();
      tokens_.push_back({pos, element_counter, CLOSE_ARRAY});

      element_counter = counters_.back();
      ++element_counter;
      counters_.pop_back();

      ++pos;
      ++objects_;

      break;

    case ':':
      if (brace_stack_.back() != '}')
      {
        throw JSONParseException("Cannot set property outside of object context");
      }
      //        tokens_.back().type = KEY;
      ++pos;
      break;

    case ',':
      //        tokens_.push_back({pos, 0, NEW_ENTRY});
      ++pos;
      break;

    default:  // If none of the above it must be number:

      ++element_counter;
      type =
          uint8_t(byte_array::consumers::NumberConsumer<NUMBER_INT, NUMBER_FLOAT>(document, pos));
      if (type == uint8_t(-1))
      {
        throw JSONParseException("Unable to parse integer.");
      }
      tokens_.push_back({oldpos, pos - oldpos, type});
      break;
    }
  }

  if (!brace_stack_.empty())
  {
    throw JSONParseException("Object or array indicators are unbalanced.");
  }
}

}  // namespace json
}  // namespace fetch
