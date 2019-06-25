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
#include "core/logger.hpp"
#include "core/string/to_lower.hpp"
#include "core/string/trim.hpp"
#include "http/response.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace fetch {
namespace http {
namespace {

byte_array::ByteArray CopyBuffer(asio::streambuf &buffer, std::size_t length)
{
  byte_array::ByteArray data;
  data.Resize(length);

  std::istream                   is(&buffer);
  std::istreambuf_iterator<char> cit(is);

  for (std::size_t i = 0; i < length; ++i)
  {
    data[i] = static_cast<uint8_t>(*cit++);
  }

  return data;
}

}  // namespace

bool HTTPResponse::ToStream(asio::streambuf &buffer) const
{
  static const char *NEW_LINE = "\r\n";

  LOG_STACK_TRACE_POINT;

  std::ostream stream(&buffer);

  stream << "HTTP/1.1 " << ToString(status_) << NEW_LINE;

  for (auto &field : header_)
  {
    stream << field.first << ": " << field.second << NEW_LINE;
  }

  if (!header_.Has("content-length"))
  {
    stream << "content-length: " << body_.size() << NEW_LINE;
  }

  stream << NEW_LINE;

  stream << body_;

  return true;
}

bool HTTPResponse::ParseHeader(asio::streambuf &buffer, std::size_t length)
{
  header_.Clear();

  bool success = true;

  auto linear_buffer = CopyBuffer(buffer, length);

  char const *current    = reinterpret_cast<char const *>(linear_buffer.pointer());
  char const *line_start = current;

  std::size_t       line_idx              = 0;
  std::size_t const last_buffer_entry_idx = (length >= 1) ? length - 1 : 0;

  // iterate through all of the characters in the buffer with the intension to break
  // up the incoming stream into lines
  for (std::size_t idx = 0; idx < length;)
  {
    // detect the end of line sequence "\r\n"
    if ((current[0] == '\r') && (idx < last_buffer_entry_idx) && (current[1] == '\n'))
    {
      // parse the response line
      if (!ParseHeaderLine(line_idx, line_start, current))
      {
        success = false;
        break;
      }

      // advance past the new line break
      idx += 2;
      current += 2;
      line_start = current;
      ++line_idx;
      continue;
    }

    // advance both index and pointer
    ++idx;
    ++current;
  }

  return success;
}

bool HTTPResponse::ParseBody(asio::streambuf &buffer, std::size_t length)
{
  body_ = CopyBuffer(buffer, length);
  return true;
}

bool HTTPResponse::ParseFirstLine(char const *begin, char const *end)
{
  std::size_t token_idx        = 0;
  char const *token_start      = begin;
  bool        token_all_digets = true;

  for (char const *current = begin; current <= end; ++current)
  {
    if (*current == ' ')
    {
      // find the status code
      if ((token_idx == 1) && token_all_digets)
      {
        std::size_t const error_code =
            std::stoul(std::string(token_start, static_cast<std::size_t>(current - token_start)));

        status_ = FromCode(error_code);
        break;
      }

      token_start      = current + 1;
      token_all_digets = true;
      ++token_idx;
    }
    else
    {
      token_all_digets &= ((*current >= '0') && (*current <= '9'));
    }
  }

  return true;
}

bool HTTPResponse::ParseHeaderLine(std::size_t line_idx, char const *begin, char const *end)
{
  // special case: the first line needs to be handled differently
  if (line_idx == 0)
  {
    return ParseFirstLine(begin, end);
  }

  // special case: do not process blank lines
  if (begin == end)
  {
    return true;
  }

  // pointers which point to be beginning and end characters of key and value elements in the line
  char const *key_start   = begin;
  char const *key_end     = nullptr;
  char const *value_start = nullptr;
  char const *value_end   = end - 1;  // safe because we have already checked for null size

  // iterator through to find the first instance of ':'
  for (char const *current = begin; current < end; ++current)
  {
    if (*current == ':')
    {
      // located our seperator (saftey checks done afterwards)
      key_end     = current - 1;
      value_start = current + 1;
      break;
    }
  }

  // validate the pointers
  bool const have_nullptrs       = (key_end == nullptr) || (value_start == nullptr);
  bool const invalid_key_start   = (key_end < key_start);
  bool const invalid_value_start = (value_start >= value_end);

  if (have_nullptrs || invalid_key_start || invalid_value_start)
  {
    return false;
  }

  // construct the stings
  std::string key(key_start, static_cast<std::size_t>((key_end - key_start) + 1));
  std::string value(value_start, static_cast<std::size_t>((value_end - value_start) + 1));

  // trim whitespace
  string::Trim(key);
  string::Trim(value);

  // keys are case insensitive, therefore we convert them to lower case
  string::ToLower(key);

  header_.Add(key, value);

  return true;
}

}  // namespace http
}  // namespace fetch
