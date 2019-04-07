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
#include "core/json/exceptions.hpp"
#include "variant/variant.hpp"

#include <memory>
#include <stack>
#include <vector>

namespace fetch {
namespace json {

/**
 * Basic JSON parser
 */
class JSONDocument
{
public:
  enum Type
  {
    KEYWORD_TRUE  = 0,
    KEYWORD_FALSE = 1,
    KEYWORD_NULL  = 2,
    STRING        = 3,

    // TODO(private issue #566): There should be defined more integer and floating point
    // types here, however they will be relevant only when used with document
    // SCHEMA defining expected types in the document.
    NUMBER_INT   = 5,
    NUMBER_FLOAT = 6,

    OPEN_OBJECT  = 11,
    CLOSE_OBJECT = 12,
    OPEN_ARRAY   = 13,
    CLOSE_ARRAY  = 14,

    KEY = 16
  };

public:
  using ByteArray      = byte_array::ByteArray;
  using ConstByteArray = byte_array::ConstByteArray;
  using Variant        = variant::Variant;

  explicit JSONDocument(ConstByteArray const &document)
  {
    Parse(document);
  }

  JSONDocument()                     = default;
  JSONDocument(JSONDocument const &) = delete;
  JSONDocument(JSONDocument &&)      = default;
  ~JSONDocument()                    = default;

  Variant &operator[](std::size_t i)
  {
    return root()[i];
  }

  Variant const &operator[](std::size_t i) const
  {
    return root()[i];
  }

  Variant &operator[](ConstByteArray const &key)
  {
    return root()[key];
  }

  Variant const &operator[](ConstByteArray const &key) const
  {
    return root()[key];
  }

  Variant &root()
  {
    return variant_;
  }

  Variant const &root() const
  {
    return variant_;
  }

  void Parse(ConstByteArray const &document);

  JSONDocument &operator=(JSONDocument const &) = delete;
  JSONDocument &operator=(JSONDocument &&) = default;

private:
  struct JSONObject
  {
    uint64_t start = 0;
    uint64_t size  = 1;
    uint64_t i     = 0;
    uint8_t  type  = 0;
  };

  struct JSONToken
  {
    uint64_t first  = 0;
    uint64_t second = 0;
    uint8_t  type   = 0;
  };

  void        Tokenise(ConstByteArray const &document);
  static void ExtractPrimitive(Variant &variant, JSONToken const &token,
                               ConstByteArray const &document);

  std::vector<uint16_t>    counters_{};
  std::vector<JSONToken *> object_stack_{};
  std::vector<JSONToken>   tokens_{};
  Variant                  variant_{1024};
  std::size_t              objects_{0};
  std::vector<char>        brace_stack_{};
};
}  // namespace json
}  // namespace fetch
