#ifndef OEF_SERVICE_CONSTS_HPP
#define OEF_SERVICE_CONSTS_HPP

enum AEAToNodeProtocolID {
  DEFAULT = 1
};


enum AEAToNodeProtocolFn {
  REGISTER_INSTANCE = 1,
  QUERY,
  REGISTER_FOR_CALLBACKS,
  DEREGISTER_FOR_CALLBACKS,
  BUY_AEA_TO_NODE,
};

enum NodeToAEAProtocolID {
  DEFAULT_ID = 1
};

enum NodeToAEAProtocolFn {
  PING = 1,
  BUY
};

//// TODO: (`HUT`) : clean this up, the directions are reversed.
//struct FetchProtocols
//{
//  
//enum {
//  AEA_TO_NODE = 1,
//  NODE_TO_AEA = 2 
//};
//};

#endif
