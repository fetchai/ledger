#pragma once

namespace fetch 
{
namespace serializers 
{
namespace interfaces 
{

class ArrayInterface
{
public:
  ArrayInterface(MsgPackByteArrayBuffer &serializer, uint64_t size)
  : serializer_{serializer}
  , size_{std::move(size)}
  { 
    std::cout << "  - Serializing " << size << " elements." << std::endl;
  }

  template< typename T >
  void Append(T const &val)
  {
    std::cout << "  - Element " << pos_ << " / " << size_ << std::endl;
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
    {
      uint16_t tmp;
      serializer_.ReadBytes(reinterpret_cast<uint8_t *>( &tmp ), sizeof(uint16_t));
      size = static_cast< uint32_t >(tmp);
      break;
    }
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
}
}
}
