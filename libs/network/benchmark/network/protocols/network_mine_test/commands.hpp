#pragma once

namespace fetch {
namespace protocols {

struct NetworkMineTest
{
  enum
  {
    PING = 139,
    PUSH_NEW_HEADER,
    PROVIDE_HEADER
  };
};
}  // namespace protocols
}  // namespace fetch
