#include "p2ptrust.hpp"

#include <math.h>

namespace fetch
{
namespace p2p
{
  P2PTrust::trust_modifiers_type P2PTrust::trust_modifiers_ = {
/*                       LIED,          BAD_CONNECTION,      DUPLICATE,      NEW_INFORMATION  */
/* BLOCK       */ { {-10 , Nan, NaN }, {-5.0, NaN, NaN }, { 1.0, NaN,10.0 }, { 3.0, NaN,15.0 } },
/* TRANSACTION */ { {-10 , NaN, NaN }, {-5.0, NaN, NaN }, { 1.0, NaN,10.0 }, { 3.0, NaN,15.0 } },
/* PEER        */ { {-10 , NaN, NaN }, {-5.0, NaN, NaN }, { 1.0, NaN,10.0 }, {20.0, NaN,100.0} },
  };
}
}
