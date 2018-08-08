#pragma once

namespace fetch {
namespace protocols {

struct NetworkBenchmark
{
  enum
  {
    PING = 127,
    INVITE_PUSH,
    PUSH,
    PUSH_CONFIDENT,
    SEND_NEXT
  };
};
}  // namespace protocols
}  // namespace fetch
