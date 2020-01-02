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

class CharArrayBuffer : public std::streambuf
{
public:
  const std::vector<asio::mutable_buffer> &buffers;
  int                                      current;
  int                                      size;

  explicit CharArrayBuffer(std::vector<asio::mutable_buffer> const &thebuffers)
    : buffers(thebuffers)
  {
    current = 0;
    size    = 0;
    for (auto &b : buffers)
    {
      size += static_cast<int>(asio::buffer_size(b));
    }
  }

  CharArrayBuffer &write(const uint32_t &i)
  {
    union
    {
      uint32_t i;
      uint8_t  c[4];
    } buffer;

    // buffer.i = i;
    buffer.i = htonl(i);
    oflow(buffer.c[0]);
    oflow(buffer.c[1]);
    oflow(buffer.c[2]);
    oflow(buffer.c[3]);

    return *this;
  }

  CharArrayBuffer &write_little_endian(const uint32_t &i)
  {
    union
    {
      uint32_t i;
      uint8_t  c[4];
    } buffer;

    // buffer.i = i;
    buffer.i = htonl(i);
    oflow(buffer.c[3]);
    oflow(buffer.c[2]);
    oflow(buffer.c[1]);
    oflow(buffer.c[0]);

    return *this;
  }

  CharArrayBuffer &read(uint32_t &i)
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

  CharArrayBuffer &write(const int32_t &i)
  {
    union
    {
      int32_t i;
      uint8_t c[4];
    } buffer;

    // buffer.i = i;
    buffer.i = static_cast<int32_t>(htonl(static_cast<uint32_t>(i)));
    oflow(buffer.c[0]);
    oflow(buffer.c[1]);
    oflow(buffer.c[2]);
    oflow(buffer.c[3]);

    return *this;
  }

  CharArrayBuffer &read(int32_t &i)
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

  CharArrayBuffer &write(const std::string &s)
  {
    for (uint32_t i = 0; i < s.size(); i++)  // Using a "<=" ensures we also write a zero terminator
    {
      oflow(s.c_str()[i]);
    }
    return *this;
  }

  CharArrayBuffer &read(std::string &s, uint32_t length)
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
    for (int i = 0; i < size; i++)
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

  bool put_char_at(int pos, char character)
  {
    if (pos >= size)
    {
      return false;
    }
    for (auto &b : buffers)
    {
      auto c = static_cast<int>(asio::buffer_size(b));
      if (pos >= c)
      {
        pos -= c;
      }
      else
      {
        // std::cout << "pos=" << pos << " c=" << character << std::endl;
        asio::buffer_cast<unsigned char *>(b)[pos] = static_cast<unsigned char>(character);
        return true;
      }
    }
    return false;
  }

  char get_char_at(int pos)
  {
    int buf = 0;

    if (pos >= size)
    {
      return traits_type::eof();
    }

    for (auto &b : buffers)
    {
      auto c = static_cast<int>(asio::buffer_size(b));
      if (pos >= c)
      {
        pos -= c;
        buf++;
      }
      else
      {
        auto r = asio::buffer_cast<unsigned char *>(b)[pos];
        return static_cast<char>(r);
      }
    }
    return traits_type::eof();
  }

  std::streamsize RemainingSpace()
  {
    return size - current;
  }

  std::streamsize xsputn(const char_type *s, std::streamsize n) override
  {
    // std::cout << "xsputn:" << s << std::endl;
    for (int i = 0; i < n; i++)
    {
      put_char_at(current, s[i]);
      current += 1;
    }
    return n;
  };

  CharArrayBuffer::int_type sputc(CharArrayBuffer::int_type c)
  {
    // std::cout << "sputc" << std::endl;
    if (current >= size)
    {
      return traits_type::eof();
    }
    put_char_at(current, traits_type::to_char_type(c));
    current += 1;
    return c;
  }

  CharArrayBuffer::int_type oflow(CharArrayBuffer::int_type c)
  {
    // std::cout << "oflow: cur=" << current << "  size=" << size << std::endl;
    if (current >= size)
    {
      return traits_type::eof();
    }
    put_char_at(current, traits_type::to_char_type(c));
    current += 1;
    return c;
  }

  CharArrayBuffer::int_type overflow(CharArrayBuffer::int_type ch) override
  {
    // std::cout << "overflow" << std::endl;
    put_char_at(current, traits_type::to_char_type(ch));
    return 1;
  }

  CharArrayBuffer::int_type underflow() override
  {
    // std::cout << "underflow" << std::endl;
    if (current >= size)
    {
      return traits_type::eof();
    }
    return traits_type::to_int_type(get_char_at(current));
  }

  CharArrayBuffer::int_type uflow() override
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

  void advance(int amount = 1)
  {
    // std::cout << "advance: cur=" << current << "  amount=" << amount << std::endl;
    current += amount;
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

private:
  // copy ctor and assignment not implemented;
  // copying not allowed
  CharArrayBuffer(const CharArrayBuffer &) = delete;
  CharArrayBuffer &operator=(const CharArrayBuffer &) = delete;
};
