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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"
#include "core/byte_array/tokenizer/tokenizer.hpp"
#include "core/script/variant.hpp"

#include <memory>
#include <stack>
#include <vector>

namespace fetch {
namespace yml {

class YMLDocument
{
  enum Type
  {
    TOKEN,
    SPACE,
    OBJECT_NAME_MODIFIER,
    LIST_ITEM,
    BLOCK,
    CATCH_ALL
  };

  enum
  {
    PROPERTY        = 2,
    ENTRY_ALLOCATOR = 3,
    OBJECT          = 10,
    ARRAY           = 11
  };

public:
  using string_type       = byte_array::ByteArray;
  using const_string_type = byte_array::ConstByteArray;

  YMLDocument()
  {}

  YMLDocument(const_string_type const &document)
    : YMLDocument()
  {
    Parse(document);
  }

  script::Variant &operator[](std::size_t const &i)
  {
    return root()[i];
  }

  script::Variant const &operator[](std::size_t const &i) const
  {
    return root()[i];
  }

  typename script::Variant::variant_proxy_type operator[](byte_array::ConstByteArray const &key)
  {
    return root()[key];
  }

  script::Variant const &operator[](byte_array::ConstByteArray const &key) const
  {
    return root()[key];
  }

  void Parse(const_string_type const &document)
  {}

  script::Variant &root()
  {
    return variants_[0];
  }
  script::Variant const &root() const
  {
    return variants_[0];
  }

private:
  void Tokenise(const_string_type const &document)
  {
    uint64_t pos = 0;
    while (pos < document.size())
    {
      uint64_t indent_size = ConsumeIndent(document, pos);

      int mod = HandleIndent(indent_size);
      switch (mod)
      {
      case OPEN_OBJECT:
        break;
      case CLOSE_OBJECT:
        break;
      case MODIFY_OBJECT:
        break;
      }

      if (document[pos] == '-')
      {
        PushArray();  // TODO(issue 38):
        ++pos;
        indent_size = 1 + ConsumeWhitespaces(const_string_type const &document, uint64_t &pos);
        mod         = HandleIndent(indent_size);
      }

      int               type;
      const_string_type key, value;

      uint64_t start_position = pos;
      switch (document[pos])
      {
      case '"':
        type  = consumers::StringConsumer<STRING>(document, pos);
        value = document.SubArray(start_position + 1, pos - start_position - 2);

        break;
      case '|':
        type  = BLOCK;
        value = ParseBlockText(document, pos);
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '.':
        type = consumers::NumberConsumer<INTEGER, FLOAT>(document, pos);
        if (type == -1)
        {
          TODO_FAIL("Expected integer or float");
        }
        value = document.SubArray(start_position, pos - start_position);
        break;
      default:
      }

      while (pos < document.size())
      {

        switch (document[pos])
        {
        case '|':
          type  = BLOCK;
          value = break;

        case ':':
        {
          type  = PROPERTY;
          value = ParseUntilNewline(document, pos);
        }
          ++pos;
          break;
        case '-':
          break;

        case ' ':
          ++pos;
          return WHITE_SPACE;
        }
      }

      return -1;
    }
  }

  uint64_t ConsumeWhitespaces(const_string_type const &document, uint64_t &pos)
  {
    uint64_t old = pos;
    while ((pos < document.size()) && ((document[pos] == ' ') || (document[pos] == '\t')))
    {
      ++pos
    }
    return pos - old;
  }

  uint64_t ConsumeIndent(const_string_type const &document, uint64_t &pos)
  {
    uint64_t old = pos;
    while ((pos < document.size()) && (document[pos] == ' '))
    {
      ++pos
    }
    return pos - old;
  }

  const_string_type ParseBlockText(const_string_type const &document, uint64_t &pos)
  {
    while ((pos < document.size()) && (document[pos] != '\n'))
    {
      ++pos;
    }

    uint64_t block_indent_size = ConsumeIndent(document, pos);
    if (block_indent_size <= indent_level.back())
    {
      TODO_FAIL("Indent must be larger than previous indent");
    }

    string_type ret;

    while (pos < document.size())
    {
      if (ret.size() != 0)
      {
        ret.Resize(ret.size() + 1);
        ret[ret.size() - 1] = '\n';
      }

      uint64_t oldpos  = pos;
      uint64_t indsize = ConsumeIndent(document, pos);

      uint64_t prev = pos;
      while ((pos < document.size()) && (document[pos] != '\n'))
      {
        ++pos;
      }
      uint64_t size = pos - prev;

      if (pos >= document.size())
      {
        break;
      }

      if ((indsize != block_indent_size) && (size != 0))
      {
        pos = oldpos;
        break;
      }

      uint64_t pos = ret.size();
      ret.Resize(ret.size() + size);
      memcpy(ret.pointer() + pos, document.pointer() + prev, size);
    }

    return ret;
  }

  int HandleIndent(uint64_t const &i)
  {
    if (i < indent_level_.back())
    {
      indent_level_.pop_back();
      return CLOSE_OBJECT;
    }
    else if (i > indent_level_.back())
    {
      indent_level_.push_back(i);
      return OPEN_OBJECT;
    }
    return MODIFY_OBJECT
  }

  enum
  {
    CLOSE_OBJECT = 0,
    OPEN_OBJECT,
    MODIFY_OBJECT
  };

  std::vector<std::size_t>       indent_level_;
  std::vector<const_string_type> tokens_;

  script::VariantList variants_;
};
}  // namespace yml
}  // namespace fetch
