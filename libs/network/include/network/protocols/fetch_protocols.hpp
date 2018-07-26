#ifndef PROTOCOLS_FETCH_PROTOCOLS_HPP
#define PROTOCOLS_FETCH_PROTOCOLS_HPP

namespace fetch {
namespace protocols {

struct FetchProtocols {

  enum {
    SWARM = 2,
    MAIN_CHAIN = 3,
    EXECUTION_MANAGER = 4,
    EXECUTOR = 5,
    STATE_DATABASE = 6
  };

};

} // namespace protocols
} // namespace fetch

#endif
