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

#include "network/fetch_asio.hpp"

#include <cctype>
#include <iostream>
#include <list>
#include <vector>

class ConstCharArrayBuffer : public std::streambuf
{
public:
  const std::vector<asio::const_buffer> &buffers;
  uint32_t                               current;
  uint32_t                               size;

  explicit ConstCharArrayBuffer(std::vector<asio::const_buffer> const &thebuffers)
    : buffers(thebuffers)
  {
    current = 0;
    size    = 0;
    for (auto &b : buffers)
    {
      size += static_cast<uint32_t>(asio::buffer_size(b));
    }
  }

  ConstCharArrayBuffer &read(uint32_t &i)
  {
    union
    {
      uint32_t i;
      uint8_t  c[4];
    } buffer;

    buffer.c[0] = static_cast<uint8_t>(uflow());
    buffer.c[1] = static_cast<uint8_t>(uflow());
    buffer.c[2] = static_cast<uint8_t>(uflow());
    buffer.c[3] = static_cast<uint8_t>(uflow());
    i           = ntohl(buffer.i);
    // i = buffer.i;

    return *this;
  }

  ConstCharArrayBuffer &read_little_endian(uint32_t &i)
  {
    union
    {
      uint32_t i;
      uint8_t  c[4];
    } buffer;

    buffer.c[3] = static_cast<uint8_t>(uflow());
    buffer.c[2] = static_cast<uint8_t>(uflow());
    buffer.c[1] = static_cast<uint8_t>(uflow());
    buffer.c[0] = static_cast<uint8_t>(uflow());
    i           = ntohl(buffer.i);
    // i = buffer.i;

    return *this;
  }

  ConstCharArrayBuffer &read(int32_t &i)
  {
    union
    {
      int32_t i;
      uint8_t c[4];
    } buffer;

    buffer.c[0] = static_cast<uint8_t>(uflow());
    buffer.c[1] = static_cast<uint8_t>(uflow());
    buffer.c[2] = static_cast<uint8_t>(uflow());
    buffer.c[3] = static_cast<uint8_t>(uflow());
    i           = static_cast<int32_t>(ntohl(static_cast<uint32_t>(buffer.i)));
    // i = buffer.i;

    return *this;
  }

  ConstCharArrayBuffer &read(std::string &s, uint32_t length)
  {
    std::string output(length, ' ');
    for (char &i : output)
    {
      i = traits_type::to_char_type(uflow());
    }

    uflow();  // discard the zero terminator.
    s = output;
    return *this;
  }

  static void diagnostic(void *p, unsigned int sz)
  {
    for (uint32_t i = 0; i < sz; i++)
    {
      char c = (static_cast<char *>(p))[i];

      switch (c)
      {
      case 10:
        std::cout << "\\n";
        break;
      default:
        if (::isprint(c) != 0)
        {
          std::cout << c;
        }
        else
        {
          int cc = static_cast<unsigned char>(c);
          std::cout << "\\x";
          for (int j = 0; j < 2; j++)
          {
            std::cout << "0123456789ABCDEF"[((cc & 0xF0) >> 4)];
            cc <<= 4;
          }
        }
      }
    }
    std::cout << std::endl;
  }

  void diagnostic()
  {
    for (uint32_t i = 0; i < size; i++)
    {
      char c = get_char_at(i);

      switch (c)
      {
      case 10:
        std::cout << "\\n";
        break;
      case 9:
        std::cout << "\\t";
        break;
      default:
        if (::isprint(c) != 0)
        {
          std::cout << c;
        }
        else
        {
          int cc = static_cast<unsigned char>(c);
          std::cout << "\\x";
          for (int j = 0; j < 2; j++)
          {
            std::cout << "0123456789abcdef"[((cc & 0xF0) >> 4)];
            cc <<= 4;
          }
        }
      }
    }
    std::cout << std::endl;
  }

  char get_char_at(uint32_t pos)
  {
    int buf = 0;

    if (pos >= size)
    {
      return traits_type::eof();
    }

    for (auto &b : buffers)
    {
      auto c = static_cast<uint32_t>(asio::buffer_size(b));
      if (pos >= c)
      {
        pos -= c;
        buf++;
      }
      else
      {
        auto r = asio::buffer_cast<const unsigned char *>(b)[pos];
        return static_cast<char>(r);
      }
    }
    return traits_type::eof();
  }

  std::streamsize RemainingData()
  {
    return size - current;
  }

  ConstCharArrayBuffer::int_type underflow() override
  {
    // std::cout << "underflow" << std::endl;
    if (current >= size)
    {
      return traits_type::eof();
    }
    return traits_type::to_int_type(get_char_at(current));
  }

  ConstCharArrayBuffer::int_type uflow() override
  {
    // std::cout << "uflow" << std::endl;
    if (current >= size)
    {
      return traits_type::eof();
    }
    auto r = traits_type::to_int_type(get_char_at(current));
    current += 1;
    return r;
  }

  void advance(std::size_t amount = 1)
  {
    // std::cout << "advance: cur=" << current << "  amount=" << amount << std::endl;
    current += static_cast<uint32_t>(amount);
  }

  std::streamsize showmanyc() override
  {
    // std::cout << "showmanyc" << std::endl;
    return static_cast<std::streamsize>(size);
  }
  int_type pbackfail(int_type ch) override
  {
    // std::cout << "pbackfail" << std::endl;
    if ((current == 0) || (ch != traits_type::eof() && ch != get_char_at(current - 1)))
    {
      return traits_type::eof();
    }
    current--;
    return traits_type::to_int_type(get_char_at(current));
  }

  ConstCharArrayBuffer(const ConstCharArrayBuffer &other) = default;

  ConstCharArrayBuffer(const ConstCharArrayBuffer &other, uint32_t sizelimit)
    : std::streambuf(other)
    , buffers(other.buffers)
    , current(other.current)
    , size(sizelimit)
  {}

  uint32_t tell() const
  {
    return current;
  }

  std::string CopyOut()
  {
    std::cout << "CopyOut" << current << "," << size << std::endl;
    std::string r;
    while (current < size)
    {
      r += get_char_at(current++);
    }
    return r;
  }
};
