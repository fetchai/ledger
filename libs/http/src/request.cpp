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

#include "http/request.hpp"

namespace fetch {
namespace http {

bool HTTPRequest::ParseBody(asio::streambuf &buffer)
{
  LOG_STACK_TRACE_POINT;

  // TODO(issue 35): Handle encoding
  body_data_ = byte_array::ByteArray();
  body_data_.Resize(content_length());
  if (buffer.size() < body_data_.size())
  {
    is_valid_ = false;
    return false;
  }

  std::istream                   is(&buffer);
  std::istreambuf_iterator<char> cit(is);

  for (std::size_t i = 0; i < body_data_.size(); ++i)
  {
    body_data_[i] = uint8_t(*cit);
    ++cit;
  }
  return true;
}

bool HTTPRequest::ParseHeader(asio::streambuf &buffer, std::size_t const &end)
{
  LOG_STACK_TRACE_POINT;

  header_data_ = byte_array::ByteArray();
  header_data_.Resize(end);

  std::istream is(&buffer);

  if (buffer.size() < end)
  {
    TODO_FAIL("trying to extract more than possible");
  }

  std::istreambuf_iterator<char> cit(is);

  std::size_t           last_pos = 0, split_key_at = 0, split_val_at = 0;
  std::size_t           line = 0;
  byte_array::ByteArray key, value, start_line;

  // loop through the header contents character by character
  for (std::size_t i = 0; i < end; ++i)
  {
    char c = *cit;
    ++cit;

    header_data_[i] = uint8_t(c);

    // find either the header separator ':' (in the case of KEY: VALUE header) to the new line
    // which indicated the value is complete
    switch (c)
    {
    case ':':
      if (split_key_at == 0)
      {
        split_val_at = split_key_at = i;
        ++split_val_at;

        // consume trailing whitespace
        while ((i + 1 < end) && (*cit) == ' ')
        {
          ++split_val_at;
          ++i;
          ++cit;
        }
      }
      break;
    case '\n':
      last_pos     = i + 1;
      split_key_at = 0;
      break;
    case '\r':
      if (last_pos != i)
      {
        if (line > 0)
        {
          // extract the key
          key = header_data_.SubArray(last_pos, split_key_at - last_pos);

          // convert to lowercase
          for (std::size_t t = 0; t < key.size(); ++t)
          {
            char &cc = reinterpret_cast<char &>(key[t]);
            if (('A' <= cc) && (cc <= 'Z'))
            {
              cc = char(cc + 'a' - 'A');
            }
          }

          ++split_key_at;

          // extract the value
          value = header_data_.SubArray(split_val_at, i - split_val_at);

          // special case: content-length extract and cache the value
          if (key == "content-length")
          {
            content_length_ = uint64_t(value.AsInt());
          }

          // TODO(issue 413): Compliance to HTTP Standard - `value` can be structured.
          if (key == "content-type")
          {
            auto const pos = value.Find(';', 0);
            if (pos != byte_array::ConstByteArray::NPOS)
            {
              value = value.SubArray(0, pos);
            }
          }
          // update header map
          header_.Add(key, value);
        }
        else
        {
          start_line = header_data_.SubArray(0, i);
        }

        ++line;
      }

      break;
    }
  }

  // since the start line if different this must be parse differently
  bool success{false};
  if (!start_line.empty())
  {
    // attempt to parse the start line
    success = ParseStartLine(start_line);
  }

  return success;
}

bool HTTPRequest::ToStream(asio::streambuf &buffer, std::string const &host, uint16_t port) const
{
  static char const *NEW_LINE = "\r\n";

  std::ostream stream(&buffer);

  stream << ToString(method_) << ' ' << uri_ << " HTTP/1.1" << NEW_LINE << "Host: " << host;

  if (port != 80)
  {
    stream << ':' << port;
  }

  stream << NEW_LINE;

  for (auto const &element : header_)
  {
    stream << element.first << ": " << element.second << NEW_LINE;
  }

  if (body_data_.size() && (!header_.Has("content-length")))
  {
    stream << "content-length: " << body_data_.size() << NEW_LINE;
  }

  stream << NEW_LINE;

  stream << body_data_;

  return true;
}

bool HTTPRequest::ParseStartLine(byte_array::ByteArray &line)
{
  LOG_STACK_TRACE_POINT;

  std::size_t i = 0;
  while (line[i] != ' ')
  {
    if (i >= line.size())
    {
      is_valid_ = false;
      return false;
    }

    char &cc = reinterpret_cast<char &>(line[i]);
    if (('A' <= cc) && (cc <= 'Z'))
    {
      cc = char(cc + 'a' - 'A');
    }
    ++i;
  }

  byte_array_type method = line.SubArray(0, i);
  FromString(method, method_);

  ++i;

  std::size_t j = i;
  while (line[i] != ' ')
  {
    if (i >= line.size())
    {
      is_valid_ = false;
      return false;
    }

    ++i;
  }

  full_uri_ = line.SubArray(j, i - j);

  // Extracting URI parameters
  std::size_t k = j;
  while (k < i)
  {
    if (line[k] == '?')
    {
      break;
    }
    ++k;
  }

  uri_ = line.SubArray(j, k - j);

  std::size_t           last = k + 1, equal = std::size_t(-1);
  byte_array::ByteArray key, value;

  while (k < i)
  {
    switch (line[k])
    {
    case '=':
      equal = k;
      break;
    case '&':
      equal = std::min(k, equal);
      key   = line.SubArray(last, equal - last);
      equal += (equal < k);
      value = line.SubArray(equal, k - equal);

      query_.Add(key, value);
      equal = std::size_t(-1);
      last  = k + 1;
      break;
    }
    ++k;
  }

  equal = std::min(k, equal);
  key   = line.SubArray(last, equal - last);
  equal += (equal < k);
  value = line.SubArray(equal, k - equal);
  query_.Add(key, value);

  while (line[i] == ' ')
  {
    if (i >= line.size())
    {
      is_valid_ = false;
      return false;
    }
    ++i;
  }
  protocol_ = line.SubArray(i, line.size() - i);
  for (std::size_t t = i; t < line.size(); ++t)
  {
    char &cc = reinterpret_cast<char &>(line[t]);
    if (('A' <= cc) && (cc <= 'Z'))
    {
      cc = char(cc + 'a' - 'A');
    }
  }

  return true;
}

}  // namespace http
}  // namespace fetch
