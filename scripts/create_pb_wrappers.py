import os
import glob
BASE = """#pragma once

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Werror"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wnested-anon-types"
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Werror"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wpedantic"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#endif

#include "%s.pb.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace fetch {
namespace oef {

}  // namespace oef
}  // namespace fetch"""

for f in glob.glob("libs/oef-messages/protos/*.proto"):
    _, name = f.rsplit("/", 1)
    name = name.replace(".proto", "")

    with open("libs/oef-messages/include/oef-messages/%s.hpp" % name, "w") as fb:
      fb.write(BASE % (name))