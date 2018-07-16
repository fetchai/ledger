#ifndef PROTOCOLS_MAIN_CHAIN_COMMANDS_HPP
#define PROTOCOLS_MAIN_CHAIN_COMMANDS_HPP

namespace fetch
{
namespace ledger
{

struct MainChain
{
enum
{
  GET_HEADER         = 1,
  GET_HEAVIEST_CHAIN = 2,
  BLOCK_PUBLISH      = 3,
};
};
}
}

#endif
