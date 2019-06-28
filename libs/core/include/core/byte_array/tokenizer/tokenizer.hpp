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

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/tokenizer/token.hpp"
#include <functional>
#include <map>
#include <vector>

namespace fetch {
namespace byte_array {

class Tokenizer : public std::vector<Token>
{
public:
  using byte_array_type        = ConstByteArray;
  using consumer_function_type = std::function<int(byte_array_type const &, uint64_t &)>;
  using indexer_function_type =
      std::function<int(byte_array_type const &, uint64_t const &, int const &)>;

  static constexpr char const *LOGGING_NAME = "Tokenizer";

  void SetConsumerIndexer(indexer_function_type function)
  {
    indexer_ = function;
  }

  std::size_t AddConsumer(consumer_function_type function)
  {
    std::size_t ret = consumers_.size();
    consumers_.push_back(function);
    return ret;
  }

  bool Parse(byte_array_type const &contents, bool clear = true)
  {
    uint64_t                           pos        = 0;
    int                                line       = 0;
    uint64_t                           char_index = 0;
    byte_array_type::value_type const *str        = contents.pointer();
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
  std::vector<consumer_function_type> consumers_;
  indexer_function_type               indexer_;
};
}  // namespace byte_array
}  // namespace fetch
