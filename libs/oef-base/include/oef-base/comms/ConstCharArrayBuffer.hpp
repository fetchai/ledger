#pragma once

#include "network/fetch_asio.hpp"

#include <ctype.h>
#include <iostream>
#include <list>
#include <vector>

class ConstCharArrayBuffer : public std::streambuf
{
public:
  const std::vector<asio::const_buffer> &buffers;
  uint32_t                               current;
  uint32_t                               size;

  ConstCharArrayBuffer(const std::vector<asio::const_buffer> &thebuffers)
    : buffers(thebuffers)
  {
    current = 0;
    size    = 0;
    for (auto &b : buffers)
    {
      size += asio::buffer_size(b);
    }
  }

  ConstCharArrayBuffer &read(uint32_t &i)
  {
    union
    {
      uint32_t i;
      uint8_t  c[4];
    } buffer;

    buffer.c[0] = (uint8_t)uflow();
    buffer.c[1] = (uint8_t)uflow();
    buffer.c[2] = (uint8_t)uflow();
    buffer.c[3] = (uint8_t)uflow();
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

    buffer.c[3] = (uint8_t)uflow();
    buffer.c[2] = (uint8_t)uflow();
    buffer.c[1] = (uint8_t)uflow();
    buffer.c[0] = (uint8_t)uflow();
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

    buffer.c[0] = (uint8_t)uflow();
    buffer.c[1] = (uint8_t)uflow();
    buffer.c[2] = (uint8_t)uflow();
    buffer.c[3] = (uint8_t)uflow();
    i           = static_cast<int32_t>(ntohl(buffer.i));
    // i = buffer.i;

    return *this;
  }

  ConstCharArrayBuffer &read(std::string &s, uint32_t length)
  {
    std::string output(length, ' ');
    for (uint32_t i = 0; i < output.size(); i++)
    {
      output[i] = traits_type::to_char_type(uflow());
    }

    uflow();  // discard the zero terminator.
    s = output;
    return *this;
  }

  static void diagnostic(void *p, unsigned int sz)
  {
    for (uint32_t i = 0; i < sz; i++)
    {
      char c = ((char *)(p))[i];

      switch (c)
      {
      case 10:
        std::cout << "\\n";
        break;
      default:
        if (::isprint(c))
        {
          std::cout << c;
        }
        else
        {
          int cc = (unsigned char)c;
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
        if (::isprint(c))
        {
          std::cout << c;
        }
        else
        {
          int cc = (unsigned char)c;
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

  ConstCharArrayBuffer::int_type underflow()
  {
    // std::cout << "underflow" << std::endl;
    if (current >= size)
    {
      return traits_type::eof();
    }
    return traits_type::to_int_type(get_char_at(current));
  }

  ConstCharArrayBuffer::int_type uflow()
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
    current += amount;
  }

  std::streamsize showmanyc()
  {
    // std::cout << "showmanyc" << std::endl;
    return (std::streamsize)size;
  }
  int_type pbackfail(int_type ch)
  {
    // std::cout << "pbackfail" << std::endl;
    if ((current == 0) || (ch != traits_type::eof() && ch != get_char_at(current - 1)))
    {
      return traits_type::eof();
    }
    current--;
    return traits_type::to_int_type(get_char_at(current));
  }

  ConstCharArrayBuffer(const ConstCharArrayBuffer &other)
    : buffers(other.buffers)
    , current(other.current)
    , size(other.size)
  {}

  ConstCharArrayBuffer(const ConstCharArrayBuffer &other, uint32_t sizelimit)
    : buffers(other.buffers)
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
    std::string r = "";
    while (current < size)
    {
      r += get_char_at(current++);
    }
    return r;
  }

private:
  // copy ctor and assignment not implemented;
  // copying not allowed
  ConstCharArrayBuffer &operator=(const ConstCharArrayBuffer &);
};
