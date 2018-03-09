#ifndef OEF_SERVICE_CONSTS_HPP
#define OEF_SERVICE_CONSTS_HPP

// TODO: (`HUT`) : refactor these into a single struct
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

#endif
