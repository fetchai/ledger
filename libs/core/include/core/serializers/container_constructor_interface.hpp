#pragma once

namespace fetch 
{
namespace serializers 
{
namespace interfaces 
{
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

    std::cout << "Creating container with " << count << " elements." << std::endl;
    uint8_t opcode = 0;
    if(count < 16)
    {
      opcode = static_cast<uint8_t>( CODE_FIXED | (count & 0xF) );
      serializer_.Allocate(sizeof(opcode));
      serializer_.WriteBytes(&opcode, sizeof(opcode));
    }
    else if(count < (1<<16))
    {
      opcode = static_cast<uint8_t>( CODE16  );
      serializer_.Allocate(sizeof(opcode));
      serializer_.WriteBytes(&opcode, sizeof(opcode));

      uint16_t size = static_cast<uint16_t>(count);
      serializer_.Allocate(sizeof(size));      
      serializer_.WriteBytes(reinterpret_cast<uint8_t *>( &size ), sizeof(size));      
    }
    else if(count < (1ull<<32))    
    {
      opcode = static_cast<uint8_t>( CODE32 );
      serializer_.Allocate(sizeof(opcode));
      serializer_.WriteBytes(&opcode, sizeof(opcode));

      serializer_.Allocate(sizeof(count));
      serializer_.WriteBytes(reinterpret_cast<uint8_t *>( &count ), sizeof(count));
    }
    else
    {
      throw SerializableException(error::TYPE_ERROR,
              std::string("Cannot create container type with more than 1 << 32 elements"));   
    }
    std::cout << "  - OPCODE: " << int(opcode) << std::endl;

    created_ = true;
    return Type(serializer_, count);
  }
private:
  bool created_{false};
  MsgPackByteArrayBuffer &serializer_;
};

}
}
}
