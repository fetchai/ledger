#ifndef OEF_SERVICE_CONSTS_HPP
#define OEF_SERVICE_CONSTS_HPP

enum AEAProtocol {
  REGISTER_INSTANCE = 1,
  QUERY,
  REGISTER_FOR_CALLBACKS,
  BUY_AEA_TO_NODE,
};

enum AEAProtocolEnum {
  DEFAULT = 1
};

enum NodeToAEAProtocol {
  PING = 1,
  BUY
};

// TODO: (`HUT`) : clean this up, the directions are reversed.
struct FetchProtocols
{
  
enum {
  AEA_TO_NODE = 1,
  NODE_TO_AEA = 2 
};
};

#endif
