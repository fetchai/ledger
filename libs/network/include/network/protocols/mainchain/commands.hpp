#pragma once

namespace fetch {
namespace ledger {

struct MainChain
{
  enum
  {
    GET_HEADER         = 1,
    GET_HEAVIEST_CHAIN = 2,
  };
};
}  // namespace ledger
}  // namespace fetch
