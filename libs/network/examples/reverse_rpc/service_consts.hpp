#ifndef SERVICE_CONSTS
#define SERVICE_CONSTS
struct AEAToNode
{

  enum
  {
    REGISTER = 1
  };
};

struct NodeToAEA
{

  enum
  {
    SEARCH = 1
  };
};

struct FetchProtocols
{

  enum
  {
    AEA_TO_NODE = 1,
    NODE_TO_AEA = 2
  };
};

#endif
