#include "network/p2pservice/manifest.hpp"

namespace fetch {

namespace network {

  const char *Manifest::service_type_names[] =
  {
    "MAINCHAIN",
    "LANE",
    "P2P",
    "HTTP",
    0
  };

}
}
