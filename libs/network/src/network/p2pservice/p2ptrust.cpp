#include "network/p2pservice/p2ptrust.hpp"

#include <cmath>

namespace fetch {
namespace p2p {
const trust_modifiers_type trust_modifiers_ = {{
    /*                       LIED,          BAD_CONNECTION,      DUPLICATE,      NEW_INFORMATION  */
    /* BLOCK       */ {{{-20.0, NAN, NAN}, {-5.0, NAN, NAN}, {1.0, NAN, 10.0}, {3.0, NAN, 15.0}}},
    /* TRANSACTION */ {{{-20.0, NAN, NAN}, {-5.0, NAN, NAN}, {1.0, NAN, 10.0}, {3.0, NAN, 15.0}}},
    /* PEER        */ {{{-20.0, NAN, NAN}, {-5.0, NAN, NAN}, {1.0, NAN, 10.0}, {20.0, NAN, 100.0}}},
}};
}
}  // namespace fetch
