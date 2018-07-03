#include"network/service/server.hpp"
namespace fetch {


struct NodeDetails 
{
  byte_array::ByteArray public_key;
  bool authenticated = false;
};

}

