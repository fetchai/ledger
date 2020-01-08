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

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/tokenizer/token.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace fetch {
namespace byte_array {

class Tokenizer : public std::vector<Token>
{
public:
  using ByteArrayType        = ConstByteArray;
  using ConsumerFunctionType = std::function<int(ByteArrayType const &, uint64_t &)>;
  using IndexerFunctionType  = std::function<int(ByteArrayType const &, uint64_t, int const &)>;

  std::size_t AddConsumer(ConsumerFunctionType function)
  {
    std::size_t ret = consumers_.size();
    consumers_.emplace_back(std::move(function));
    return ret;
  }

  bool Parse(ByteArrayType const &contents, bool clear = true)
  {
    uint64_t                        pos        = 0;
    int                             line       = 0;
    uint64_t                        char_index = 0;
    ByteArrayType::ValueType const *str        = contents.pointer();
    if (clear)
    {
      this->clear();
    }

    // Counting tokens
    if (contents.size() > 100000)
    {  // TODO(issue 37): Optimise this parameter

      std::size_t n = 0;
      while (pos < contents.size())
      {
        uint64_t oldpos     = pos;
        int      token_type = 0;

        if (indexer_)
        {
          int  index = 0, prev_index = 0;
          bool check = true;
          while (check)
          {
            index = indexer_(contents, pos, prev_index);

            auto &c = consumers_[std::size_t(index)];

            pos        = oldpos;
            token_type = c(contents, pos);
            if (token_type > -1)
            {
              break;
            }

            check      = (index != prev_index);
            prev_index = index;
          }
        }
        else
        {
          for (auto &c : consumers_)
          {
            pos        = oldpos;
            token_type = c(contents, pos);
            if (token_type > -1)
            {
              break;
            }
          }
        }
        if (pos == oldpos)
        {
          TODO_FAIL("Unable to parse char on ", pos, "  '", str[pos], "'", ", '", contents[pos],
                    "'");
        }
        ++n;
      }

      this->reserve(n);
    }

    // Extracting tokens
    pos = 0;
    while (pos < contents.size())
    {
      uint64_t oldpos     = pos;
      int      token_type = 0;

      if (indexer_)
      {
        int  index = 0, prev_index = -1;
        bool check = true;
        while (check)
        {
          index = indexer_(contents, pos, prev_index);

          auto &c = consumers_[std::size_t(index)];

          pos        = oldpos;
          token_type = c(contents, pos);
          if (token_type > -1)
          {
            break;
          }

          check      = (index != prev_index);
          prev_index = index;
        }
      }
      else
      {
        for (auto &c : consumers_)
        {
          pos        = oldpos;
          token_type = c(contents, pos);
          if (token_type > -1)
          {
            break;
          }
        }
      }

      if (pos == oldpos)
      {
        TODO_FAIL("Unable to parse char on ", pos, "  '", str[pos], "'", ", '", contents[pos], "'");
      }

      this->emplace_back(contents, oldpos, pos - oldpos);
      Token &tok = this->back();

      tok.SetLine(line);
      tok.SetChar(char_index);
      tok.SetType(token_type);

      for (uint64_t i = oldpos; i < pos; ++i)
      {
        ++char_index;
        if (str[i] == '\n')
        {
          ++line;
          char_index = 0;
        }
      }
    }

    return true;
  }

private:
  std::vector<ConsumerFunctionType> consumers_;
  IndexerFunctionType               indexer_;
};
}  // namespace byte_array
}  // namespace fetch
