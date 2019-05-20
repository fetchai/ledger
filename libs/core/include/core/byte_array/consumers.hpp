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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/tokenizer/tokenizer.hpp"

#include <cctype>

#include <emmintrin.h>
namespace fetch {
namespace byte_array {
namespace consumers {
/* Consumes an integer from a byte array if found.
 * @param str is a constant byte array.
 * @param pos is a position in the byte array.
 *
 * The implementation follows the details given on JSON.org. The
 * function has support for numbers such as 23, 32.15, -2e0 and
 * -3.2e+3. Numbers are classified either as integers or floating
 * points.
 */
template <int NUMBER_INT, int NUMBER_FLOAT = NUMBER_INT>
int NumberConsumer(byte_array::ConstByteArray const &str, uint64_t &pos)
{
  /* ┌┐ ┌┐
  ** ││                   ┌──────────────────────┐ ││
  ** ││                   │                      │ ││
  ** ││                   │                      │ ││
  ** ││             .─.   │   .─.    .─────.     │ ││
  ** │├─┬────────┬▶( 0 )──┼─▶( - )─▶( digit
  *)──┬─┴─┬─────┬────────────┬───────▶││
  ** ││ │        │  `─' ┌─┘   `─'    `─────'   │   │     │            │ ││
  ** ││ │        │      │               ▲      │   ▼     ▼         .─────. ││
  ** ││ │   .─.  │   .─────.            │      │  .─.   .─.  ┌───▶( digit )──┐
  *││
  ** └┘ └─▶( - )─┴─▶( digit )──┐        └──────┘ ( e ) ( E ) │     `─────'   │
  *└┘
  **        `─'      `─────'   │                  `─'   `─'  │        ▲      │
  **                    ▲      │                   │     │   │        │      │
  **                    │      │                   ├──┬──┤   │        └──────┘
  **                    └──────┘                   ▼  │  ▼   │
  **                                              .─. │ .─.  │
  **                                             ( + )│( - ) │
  **                                              `─' │ `─'  │
  **                                               │  │  │   │
  **                                               └──┼──┘   │
  **                                                  └──────┘
  */
  uint64_t oldpos = pos;
  uint64_t N      = pos + 1;
  if ((N < str.size()) && (str[pos] == '-') && ('0' <= str[N]) && (str[N] <= '9'))
  {
    pos += 2;
  }

  while ((pos < str.size()) && ('0' <= str[pos]) && (str[pos] <= '9'))
  {
    ++pos;
  }
  if (pos != oldpos)
  {
    int ret = int(NUMBER_INT);

    if ((pos < str.size()) && (str[pos] == '.'))
    {
      ++pos;
      ret = int(NUMBER_FLOAT);
      while ((pos < str.size()) && ('0' <= str[pos]) && (str[pos] <= '9'))
      {
        ++pos;
      }
    }

    if ((pos < str.size()) && ((str[pos] == 'e') || (str[pos] == 'E')))
    {
      uint64_t rev = 1;
      ++pos;

      if ((pos < str.size()) && ((str[pos] == '-') || (str[pos] == '+')))
      {
        ++pos;
        ++rev;
      }

      oldpos = pos;
      while ((pos < str.size()) && ('0' <= str[pos]) && (str[pos] <= '9'))
      {
        ++pos;
      }
      if (oldpos == pos)
      {
        pos -= rev;
      }
      ret = int(NUMBER_FLOAT);
    }

    return ret;
  }

  return -1;
}

/* Consumes string starting and ending with ".
 * @param str is a constant byte array.
 * @param pos is a position in the byte array.
 *
 * The implementation follows the details given on JSON.org.
 * Currently, there is no checking if unicodes are correctly
 * formatted.
 */
template <int STRING>
int StringConsumerSSE(byte_array::ConstByteArray const &str, uint64_t &pos)
{
  if (str[pos] != '"')
  {
    return -1;
  }
  ++pos;
  if (pos >= str.size())
  {
    return -1;
  }

  uint8_t const *     ptr         = str.pointer() + pos;
  alignas(16) uint8_t compare[16] = {'"', '"', '"', '"', '"', '"', '"', '"',
                                     '"', '"', '"', '"', '"', '"', '"', '"'};

  __m128i  comp  = _mm_load_si128((__m128i *)compare);
  __m128i  mptr  = _mm_loadu_si128((__m128i *)ptr);  // TODO(issue 37): Optimise to follow alignment
  __m128i  mret  = _mm_cmpeq_epi8(comp, mptr);
  uint16_t found = uint16_t(_mm_movemask_epi8(mret));

  while ((pos < str.size()) && (!found))
  {
    pos += 16;
    ptr += 16;
    // TODO(issue 37): Handle \"x
    mptr = _mm_loadu_si128((__m128i *)ptr);
    mret = _mm_cmpeq_epi8(comp, mptr);
    found        = uint16_t(_mm_movemask_epi8(mret));
  }

  pos += uint64_t(__builtin_ctz(found));

  if (pos >= str.size())
  {
    return -1;
  }
  ++pos;
  return STRING;
}

template <int STRING>
int StringConsumer(byte_array::ConstByteArray const &str, uint64_t &pos)
{
  if (str[pos] != '"')
  {
    return -1;
  }
  ++pos;
  if (pos >= str.size())
  {
    return -1;
  }

  while ((pos < str.size()) && (str[pos] != '"'))
  {
    pos += 1 + (str[pos] == '\\');
  }

  if (pos >= str.size())
  {
    return -1;
  }
  ++pos;
  return STRING;
}

template <int TOKEN>
int Token(byte_array::ConstByteArray const &str, uint64_t &pos)
{
  uint8_t c = str[pos];

  if (!(std::isalpha(c)))
  {
    return -1;
  }
  ++pos;
  if (pos >= str.size())
  {
    return TOKEN;
  }
  c = str[pos];
  while (std::isalnum(c))
  {
    ++pos;
    if (pos >= str.size())
    {
      break;
    }
    c = str[pos];
  }
  return TOKEN;
}

template <int CATCH_ALL>
int AnyChar(byte_array::ConstByteArray const & /*str*/, uint64_t &pos)
{
  ++pos;
  return CATCH_ALL;
}
}  // namespace consumers
}  // namespace byte_array
}  // namespace fetch
