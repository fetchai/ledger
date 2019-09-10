#pragma once

#include "boost/asio.hpp"
#include "boost/asio/write.hpp"
#include "boost/asio/buffer.hpp"

#include <iostream>
#include <list>
#include <vector>
#include <ctype.h>

class CharArrayBuffer : public std::streambuf
{
public:
  const std::vector<boost::asio::mutable_buffer> &buffers;
  int current;
  int size;

  CharArrayBuffer(const std::vector<boost::asio::mutable_buffer> &thebuffers):buffers(thebuffers)
  {
    current=0;
    size = 0;
    for(auto &b :  buffers)
    {
      size += boost::asio::buffer_size(b);
    }
  }

  CharArrayBuffer& write(const uint32_t &i)
  {
    union {
      uint32_t i;
      uint8_t c[4];
    } buffer;

    //buffer.i = i;
    buffer.i = htonl(i);
    oflow(buffer.c[0]);
    oflow(buffer.c[1]);
    oflow(buffer.c[2]);
    oflow(buffer.c[3]);

    return *this;
  }

  CharArrayBuffer& write_little_endian(const uint32_t &i)
  {
    union {
      uint32_t i;
      uint8_t c[4];
    } buffer;

    //buffer.i = i;
    buffer.i = htonl(i);
    oflow(buffer.c[3]);
    oflow(buffer.c[2]);
    oflow(buffer.c[1]);
    oflow(buffer.c[0]);

    return *this;
  }

  CharArrayBuffer& read(uint32_t &i)
  {
    union {
      uint32_t i;
      uint8_t c[4];
    } buffer;

    buffer.c[0] = (uint8_t)uflow();
    buffer.c[1] = (uint8_t)uflow();
    buffer.c[2] = (uint8_t)uflow();
    buffer.c[3] = (uint8_t)uflow();
    i = ntohl(buffer.i);
    //i = buffer.i;

    return *this;
  }

  CharArrayBuffer& write(const int32_t &i)
  {
    union {
      int32_t i;
      uint8_t c[4];
    } buffer;

    //buffer.i = i;
    buffer.i = static_cast<int32_t>(htonl(i));
    oflow(buffer.c[0]);
    oflow(buffer.c[1]);
    oflow(buffer.c[2]);
    oflow(buffer.c[3]);

    return *this;
  }

  CharArrayBuffer& read(int32_t &i)
  {
    union {
      int32_t i;
      uint8_t c[4];
    } buffer;

    buffer.c[0] = (uint8_t)uflow();
    buffer.c[1] = (uint8_t)uflow();
    buffer.c[2] = (uint8_t)uflow();
    buffer.c[3] = (uint8_t)uflow();
    i = static_cast<int32_t>(ntohl(buffer.i));
    //i = buffer.i;

    return *this;
  }

  CharArrayBuffer& write(const std::string &s)
  {
    for(uint32_t i=0;i<s.size();i++)  // Using a "<=" ensures we also write a zero terminator
    {
      oflow(s.c_str()[i]);
    }
    return *this;
  }

  CharArrayBuffer& read(std::string &s, uint32_t length)
  {
    std::string output(length, ' ');
    for(uint32_t i=0;i<output.size();i++)
    {
      output[i] = traits_type::to_char_type(uflow());
    }

    uflow(); // discard the zero terminator.
    s = output;
    return *this;
  }

  static void diagnostic(void *p, unsigned int sz)
  {
    for(uint32_t i=0;i<sz;i++)
    {
      char c = ((char*)(p))[i];

      switch(c)
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
            for(int j=0;j<2;j++)
            {
              std::cout << "0123456789ABCDEF"[((cc&0xF0) >> 4)];
              cc <<= 4;
            }
          }
      }
    }
    std::cout << std::endl;
  }

  void diagnostic()
  {
    for(int i=0;i<size;i++)
    {
      char c = get_char_at(i);

      switch(c)
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
            for(int j=0;j<2;j++)
            {
              std::cout << "0123456789abcdef"[((cc&0xF0) >> 4)];
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
    for(auto &b :  buffers)
    {
      int c = static_cast<int>(boost::asio::buffer_size(b));
      if (pos >= c)
      {
        pos -= c;
      }
      else
      {
         //std::cout << "pos=" << pos << " c=" << character << std::endl;
        boost::asio::buffer_cast<unsigned char*>(b)[pos] = static_cast<unsigned char>(character);
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

    for(auto &b :  buffers)
    {
      int c = static_cast<int>(boost::asio::buffer_size(b));
      if (pos >= c)
      {
        pos -= c;
        buf++;
      }
      else
      {
        auto r = boost::asio::buffer_cast<unsigned char*>(b)[pos];
        return static_cast<char>(r);
      }
    }
    return traits_type::eof();
  }

  std::streamsize remainingSpace()
  {
    return size - current;
  }

  std::streamsize xsputn(const char_type* s, std::streamsize n)
  {
    //std::cout << "xsputn:" << s << std::endl;
    for(int i = 0; i< n; i++)
    {
      put_char_at(current, s[i]);
      current += 1;
    }
    return n;
  };

  CharArrayBuffer::int_type sputc(CharArrayBuffer::int_type c)
  {
    //std::cout << "sputc" << std::endl;
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
    //std::cout << "oflow: cur=" << current << "  size=" << size << std::endl;
    if (current >= size)
    {
      return traits_type::eof();
    }
    put_char_at(current, traits_type::to_char_type(c));
    current += 1;
    return c;
  }

  CharArrayBuffer::int_type overflow(CharArrayBuffer::int_type ch)
  {
    //std::cout << "overflow" << std::endl;
    put_char_at(current, traits_type::to_char_type(ch));
    return 1;
  }

  CharArrayBuffer::int_type underflow()
  {
    //std::cout << "underflow" << std::endl;
    if (current >= size)
    {
      return traits_type::eof();
    }
    return traits_type::to_int_type(get_char_at(current));
  }

  CharArrayBuffer::int_type uflow()
  {
    //std::cout << "uflow" << std::endl;
    if (current >= size)
    {
      return traits_type::eof();
    }
    auto r = traits_type::to_int_type(get_char_at(current));
    current += 1;
    return r;
  }

  void advance(int amount=1)
  {
    //std::cout << "advance: cur=" << current << "  amount=" << amount << std::endl;
    current += amount;
  }

  std::streamsize showmanyc()
  {
    //std::cout << "showmanyc" << std::endl;
    return (std::streamsize)size;
  }
  int_type pbackfail(int_type ch)
  {
    //std::cout << "pbackfail" << std::endl;
    if (
        (current == 0)
        ||
        (ch != traits_type::eof() && ch != get_char_at(current-1))
        )
    {
      return traits_type::eof();
    }
    current--;
    return traits_type::to_int_type(get_char_at(current));
  }
private:

  // copy ctor and assignment not implemented;
  // copying not allowed
  CharArrayBuffer(const CharArrayBuffer &);
  CharArrayBuffer &operator= (const CharArrayBuffer &);
};
