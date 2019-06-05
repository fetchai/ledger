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
#include "core/byte_array/const_byte_array.hpp"
#include "core/logger.hpp"
#include "core/serializers/counter.hpp"
#include "msg_pack_serialiser_type_definitions.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {

class MsgPackByteArrayBuffer
{
  static char const *const LOGGING_NAME;

public:
  using self_type         = MsgPackByteArrayBuffer;
  using byte_array_type   = byte_array::ByteArray;
  using size_counter_type = serializers::SizeCounter<self_type>;

  MsgPackByteArrayBuffer()                              = default;
  MsgPackByteArrayBuffer(MsgPackByteArrayBuffer &&from) = default;
  MsgPackByteArrayBuffer &operator=(MsgPackByteArrayBuffer &&from) = default;

  /**
   * @brief Contructting from MUTABLE ByteArray.
   *
   * DEEP copy is made here due to safety reasons to avoid later
   * misshaps & missunderstrandings related to what hapens with reserved
   * memory of mutable @ref s instance passed in by caller of this
   * constructor once this class starts to modify content of underlaying
   * internal @ref data_ ByteArray and then resize/reserve it.
   *
   * @param s Input mutable instance of ByteArray to copy content from (by
   *          value as explained above)
   */
  MsgPackByteArrayBuffer(byte_array::ByteArray const &s)
    : data_{s.Copy()}
  {}

  // TODO: We should implement move constructor to allow maximal efficiency

  MsgPackByteArrayBuffer(MsgPackByteArrayBuffer const &from)
    : data_{from.data_.Copy()}
    , pos_{from.pos_}
    , size_counter_{from.size_counter_}
  {}

  MsgPackByteArrayBuffer &operator=(MsgPackByteArrayBuffer const &from)
  {
    *this = MsgPackByteArrayBuffer{from};
    return *this;
  }

  void Allocate(uint64_t const &delta)
  {
    Resize(delta, ResizeParadigm::RELATIVE);
  }

  void Resize(uint64_t const &   size,
              ResizeParadigm const &resize_paradigm     = ResizeParadigm::RELATIVE,
              bool const            zero_reserved_space = true)
  {
    data_.Resize(size, resize_paradigm, zero_reserved_space);

    switch (resize_paradigm)
    {
    case ResizeParadigm::RELATIVE:
      break;

    case ResizeParadigm::ABSOLUTE:
      if (pos_ > size)
      {
        seek(size);
      }
      break;
    };
  }

  void Reserve(uint64_t const &   size,
               ResizeParadigm const &resize_paradigm     = ResizeParadigm::RELATIVE,
               bool const            zero_reserved_space = true)
  {
    data_.Reserve(size, resize_paradigm, zero_reserved_space);
  }

  void WriteBytes(uint8_t const *arr, uint64_t const &size)
  {
    data_.WriteBytes(arr, size, pos_);
    pos_ += size;
  }

  void WriteByte(uint8_t const &val)
  {
    data_.WriteBytes(&val, 1, pos_);
    ++pos_;
  }


  void ReadByte(uint8_t &val)
  {
    data_.ReadBytes(&val, 1, pos_);
    ++pos_;
  }

  void ReadBytes(uint8_t *arr, uint64_t const &size)
  {
    data_.ReadBytes(arr, size, pos_);
    pos_ += size;
  }

  void ReadByteArray(byte_array::ConstByteArray &b, uint64_t const &size)
  {
    b = data_.SubArray(pos_, size);
    pos_ += size;
  }

  void SkipBytes(uint64_t const &size)
  {
    pos_ += size;
  }

  template <typename T>
  typename IntegerSerializer<T, self_type>::DriverType &operator<<(T const &val)
  {
    IntegerSerializer<T, MsgPackByteArrayBuffer >::Serialize(*this, val);
    return *this;
  }

  template <typename T>
  typename IntegerSerializer<T, self_type>::DriverType &operator>>(T &val)
  {
    IntegerSerializer<T, MsgPackByteArrayBuffer >::Deserialize(*this, val);
    return *this;
  }

  template <typename T>
  typename BooleanSerializer<T, self_type>::DriverType &operator<<(T const &val)
  {
    BooleanSerializer<T, MsgPackByteArrayBuffer >::Serialize(*this, val);
    return *this;
  }

  template <typename T>
  typename BooleanSerializer<T, self_type>::DriverType &operator>>(T &val)
  {
    BooleanSerializer<T, MsgPackByteArrayBuffer >::Deserialize(*this, val);
    return *this;
  }


  template <typename T>
  typename StringSerializer<T, self_type>::DriverType &operator<<(T const &val)
  {
    StringSerializer<T, MsgPackByteArrayBuffer >::Serialize(*this, val);
    return *this;
  }

  template <typename T>
  typename StringSerializer<T, self_type>::DriverType &operator>>(T &val)
  {
    StringSerializer<T, MsgPackByteArrayBuffer >::Deserialize(*this, val);
    return *this;
  }

  class ArrayInterface
  {
  public:
    ArrayInterface(MsgPackByteArrayBuffer &serializer, uint64_t size)
    : serializer_{serializer}
    , size_{std::move(size)}
    { }

    template< typename T >
    void Append(T const &val)
    {
      ++pos_;
      if(pos_ > size_)
      {
        throw SerializableException(std::string("exceeded number of allocated elements in array serialization"));
      }

      serializer_ << val;
    }
  private:
    MsgPackByteArrayBuffer &serializer_;
    uint64_t size_;
    uint64_t pos_{0};
  };

  class MapInterface
  {
  public:
    MapInterface(MsgPackByteArrayBuffer &serializer, uint64_t size)
    : serializer_{serializer}
    , size_{std::move(size)}
    { }

    template< typename K, typename V >
    void Append(K const &key, V const &val)
    {
      ++pos_;
      if(pos_ > size_)
      {
        throw SerializableException(std::string("exceeded number of allocated elements in array serialization"));
      }

      serializer_ << key;
      serializer_ << val;
    }
  private:
    MsgPackByteArrayBuffer &serializer_;
    uint64_t size_;
    uint64_t pos_{0};
  };

  template< typename I, uint8_t CF , uint8_t C16 , uint8_t C32>
  class ContainerConstructorInterface
  {
  public:
    using Type = I;

    enum 
    {
      CODE_FIXED = CF,
      CODE16     = C16,
      CODE32     = C32
    };

    ContainerConstructorInterface(MsgPackByteArrayBuffer &serializer)
    : serializer_{serializer}
    { }

    Type operator()(uint64_t count)
    {
      if(created_)
      {
        throw SerializableException(std::string("Constructor is one time use only."));
      }

      if(count < 15)
      {
        uint8_t size = static_cast<uint8_t>( CODE_FIXED | (count & 15) );
        serializer_.Allocate(sizeof(size));
        serializer_.WriteBytes(&size, sizeof(size));
      }
      else if(count < ((1<<16)-1))
      {
        uint8_t opcode = static_cast<uint8_t>( CODE16  );
        serializer_.Allocate(sizeof(opcode));
        serializer_.WriteBytes(&opcode, sizeof(opcode));

        uint16_t size = static_cast<uint16_t>(count);
        serializer_.Allocate(sizeof(size));      
        serializer_.WriteBytes(reinterpret_cast<uint8_t *>( &size ), sizeof(size));      
      }
      else if(count < ((1ull<<32)-1))    
      {
        uint8_t opcode = static_cast<uint8_t>( CODE32 );
        serializer_.Allocate(sizeof(opcode));
        serializer_.WriteBytes(&opcode, sizeof(opcode));

        serializer_.Allocate(sizeof(count));
        serializer_.WriteBytes(reinterpret_cast<uint8_t *>( &count ), sizeof(count));
      }
      else
      {
        throw SerializableException(error::TYPE_ERROR,
                std::string("Cannot create maps with more than 1 << 32 elements"));   
      }


      created_ = true;
      return Type(serializer_, count);
    }
  private:
    bool created_{false};
    MsgPackByteArrayBuffer &serializer_;
  };

  template <typename T>
  typename ArraySerializer<T, self_type>::DriverType &operator<<(T const &val)
  {
    using Serializer = ArraySerializer<T, MsgPackByteArrayBuffer >;
    using Constructor = ContainerConstructorInterface<ArrayInterface, 0x90, 0xdc, 0xdd >;

    Constructor constructor(*this);
    Serializer::Serialize(constructor, val);
    return *this;
  }

  class ArrayDeserializer
  {
  public:
    enum 
    {
      CODE_FIXED =  0x90, 
      CODE16 = 0xdc, 
      CODE32 = 0xdd
    };
    ArrayDeserializer(MsgPackByteArrayBuffer &serializer)
    : serializer_{serializer}
    { 
      uint8_t opcode;
      uint32_t size;
      serializer_.ReadByte(opcode);
      switch(opcode)      
      {
      case CODE16:
        serializer_.ReadBytes(reinterpret_cast<uint8_t *>( &size ), sizeof(uint16_t));
        break;
      case CODE32:
        serializer_.ReadBytes(reinterpret_cast<uint8_t *>( &size ), sizeof(uint32_t));            
        break;
      default:
        if((opcode & CODE_FIXED) != CODE_FIXED)
        {
          throw SerializableException(std::string("incorrect size opcode for array size."));
        }
        size = static_cast< uint32_t >( opcode & 15 );
      }

      size_ = static_cast< uint64_t >(size);
    }

    template< typename V >
    void GetNextValue(V &value)
    {
      ++pos_;
      if(pos_ > size_)
      {
        throw SerializableException(std::string("tried to deserialise more fields in map than there exists."));
      }
      serializer_ >> value;
    }

    uint64_t size() const 
    {
      return size_;
    }
  private:
    MsgPackByteArrayBuffer &serializer_;
    uint64_t size_{0};
    uint64_t pos_{0};
  };

  template <typename T>
  typename ArraySerializer<T, self_type>::DriverType &operator>>(T &val)
  {
    using Serializer = ArraySerializer<T, MsgPackByteArrayBuffer >;
    ArrayDeserializer array(*this);
    Serializer::Deserialize(array, val);
    return *this;
  }  





  template <typename T>
  typename MapSerializer<T, self_type>::DriverType &operator<<(T const &val)
  {
    using Serializer = MapSerializer<T, MsgPackByteArrayBuffer >;
    using Constructor = ContainerConstructorInterface< MapInterface, 0x80, 0xde, 0xdf >;

    Constructor constructor(*this);
    Serializer::Serialize(constructor, val);
    return *this;
  }


  class MapDeserializer
  {
  public:
    enum 
    {
      CODE_FIXED =  0x80, 
      CODE16 = 0xde, 
      CODE32 = 0xdf
    };
    MapDeserializer(MsgPackByteArrayBuffer &serializer)
    : serializer_{serializer}
    { 
      uint8_t opcode;
      uint32_t size;
      serializer_.ReadByte(opcode);
      switch(opcode)      
      {
      case CODE16:
        serializer_.ReadBytes(reinterpret_cast<uint8_t *>( &size ), sizeof(uint16_t));
        break;
      case CODE32:
        serializer_.ReadBytes(reinterpret_cast<uint8_t *>( &size ), sizeof(uint32_t));            
        break;
      default:
        if((opcode & CODE_FIXED) != CODE_FIXED)
        {
          throw SerializableException(std::string("incorrect size opcode for map size."));
        }
        size = static_cast< uint32_t >( opcode & 15 );
      }

      size_ = static_cast< uint64_t >(size);
    }

    template< typename K, typename V>
    void GetNextKeyPair(K &key, V &value)
    {
      ++pos_;
      if(pos_ > size_)
      {
        throw SerializableException(std::string("tried to deserialise more fields in map than there exists."));
      }
      serializer_ >> key >> value;
    }

    uint64_t size() const 
    {
      return size_;
    }
  private:
    MsgPackByteArrayBuffer &serializer_;
    uint64_t size_{0};
    uint64_t pos_{0};
  };

  template <typename T>
  typename MapSerializer<T, self_type>::DriverType &operator>>(T &val)
  {
    using Serializer = MapSerializer<T, MsgPackByteArrayBuffer >;
    MapDeserializer map(*this);
    Serializer::Deserialize(map, val);
    return *this;
  }  


  template <typename T>
  self_type &Pack(T const *val)
  {
    return this->operator<<(val);
  }

  template <typename T>
  self_type &Pack(T const &val)
  {
    return this->operator<<(val);
  }

  template <typename T>
  self_type &Unpack(T &val)
  {
    return this->operator>>(val);
  }

  // FIXME: Incorrect naming
  void seek(uint64_t p)
  {
    pos_ = p;
  }
  uint64_t tell() const
  {
    return pos_;
  }

  uint64_t size() const
  {
    return data_.size();
  }

  uint64_t capacity() const
  {
    return data_.capacity();
  }

  int64_t bytes_left() const
  {
    return int64_t(data_.size()) - int64_t(pos_);
  }
  byte_array::ByteArray const &data() const
  {
    return data_;
  }

  template <typename... ARGS>
  self_type &Append(ARGS const &... args)
  {
    auto size_count_guard = sizeCounterGuardFactory(size_counter_);
    if (size_count_guard.is_unreserved())
    {
      size_counter_.Allocate(size());
      size_counter_.seek(tell());

      size_counter_.Append(args...);
      if (size() < size_counter_.size())
      {
        Reserve(size_counter_.size() - size());
      }
    }

    AppendInternal(args...);
    return *this;
  }

private:
  template <typename T, typename... ARGS>
  void AppendInternal(T const &arg, ARGS const &... args)
  {
    *this << arg;
    AppendInternal(args...);
  }

  void AppendInternal()
  {}

  byte_array_type   data_;
  uint64_t          pos_ = 0;
  size_counter_type size_counter_;
};



}  // namespace serializers
}  // namespace fetch
