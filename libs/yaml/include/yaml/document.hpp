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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"
#include "variant/variant.hpp"
#include "yaml/exceptions.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace fetch {
namespace yaml {

class YamlDocument
{
public:
  enum Type
  {
    KEYWORD_TRUE     = 0,
    KEYWORD_FALSE    = 1,
    KEYWORD_NULL     = 2,
    KEYWORD_CONTENT  = 3,
    KEYWORD_INF      = 4,
    KEYWORD_NEG_INF  = 5,
    KEYWORD_NAN      = 6,
    STRING           = 7,
    STRING_MULTILINE = 8,
    COMMENT          = 9,

    // TODO(private issue #566): There should be defined more integer and floating point
    // types here, however they will be relevant only when used with document
    // SCHEMA defining expected types in the document.
    NUMBER_INT   = 10,
    NUMBER_FLOAT = 11,
    NUMBER_HEX   = 12,
    NUMBER_OCT   = 13,

    OPEN_OBJECT  = 14,
    CLOSE_OBJECT = 15,
    OPEN_ARRAY   = 16,
    CLOSE_ARRAY  = 17,

    NEW_ENTRY           = 18,
    NEW_MULTILINE_ENTRY = 19,

    KEY = 20,

    ALIAS           = 21,
    ALIAS_REFERENCE = 22,
    TAG             = 23,
    KEYWORD_TAG     = 24
  };

  using ByteArray      = byte_array::ByteArray;
  using ConstByteArray = byte_array::ConstByteArray;
  using Variant        = variant::Variant;

  explicit YamlDocument(ConstByteArray const &document)
  {
    Parse(document);
  }

  YamlDocument()
  {
    // The default of a YamlDocument is to be an empty object
    variant_ = Variant::Object();
  }

  YamlDocument(YamlDocument const &) = delete;
  YamlDocument(YamlDocument &&)      = default;
  ~YamlDocument()                    = default;

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

  YamlDocument &operator=(YamlDocument const &) = delete;
  YamlDocument &operator=(YamlDocument &&) = default;

  bool Has(byte_array::ConstByteArray const &key)
  {
    return variant_.Has(key);
  }

private:
  constexpr static char const *LOGGING_NAME = "YamlDocument";

  struct YamlObject
  {
    Variant *data  = nullptr;
    uint     ident = 0;
    uint     line  = 0;
  };

  struct YamlToken
  {
    uint64_t first  = 0;
    uint64_t second = 0;
    uint8_t  type   = 0;
    uint     ident  = 0;
    uint     line   = 0;
  };

  void               Tokenise(ConstByteArray const &document);
  static void        ExtractPrimitive(Variant &variant, YamlToken const &token,
                                      ConstByteArray const &document);
  static YamlObject *FindInStack(std::vector<YamlObject> &stack, uint ident);

  std::vector<uint16_t>    counters_{};
  std::vector<YamlToken *> object_stack_{};
  std::vector<YamlToken>   tokens_{};
  Variant                  variant_{1024};
  std::size_t              objects_{0};
  std::vector<char>        brace_stack_{};
};

}  // namespace yaml
}  // namespace fetch
