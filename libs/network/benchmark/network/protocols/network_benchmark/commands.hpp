#ifndef PROTOCOLS_NETWORK_BENCHMARK_COMMANDS_HPP
#define PROTOCOLS_NETWORK_BENCHMARK_COMMANDS_HPP

namespace fetch
{
namespace protocols
{

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
}
}

#endif
