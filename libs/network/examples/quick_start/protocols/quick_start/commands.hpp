#ifndef PROTOCOLS_QUICK_START_COMMANDS_HPP
#define PROTOCOLS_QUICK_START_COMMANDS_HPP

namespace fetch {
namespace protocols {

struct QuickStart
{
  enum
  {
    PING = 127,
    SEND_MESSAGE,
    SEND_DATA
  };
};
}  // namespace protocols
}  // namespace fetch

#endif
