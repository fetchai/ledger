//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "network/p2pservice/p2ptrust_bayrank.hpp"

#include <cmath>

namespace fetch {
namespace p2p {

std::ostream& operator<<(std::ostream& o, const fetch::p2p::TrustQuality& q)
{
  o << ToString(q);
  return o;
}

using Gaussian = math::statistics::Gaussian<double>;

ReferencePlayers const reference_players_{
    {
      TrustSubject :: BLOCK, TrustQualityArrayGaussian
      {
        {/* LIED             */ Gaussian::ClassicForm(0, 100 / 24.),
         /* BAD_CONNECTION   */ Gaussian::ClassicForm(50, 100 / 2.),
         /* DUPLICATE        */ Gaussian::ClassicForm(80, 100 / 6.),
         /* NEW_INFORMATION  */ Gaussian::ClassicForm(100, 100 / 6.),
         /* NEW_PEER         */ Gaussian::ClassicForm(100, 100 / 6.)
        }
      }
    },
    {
      TrustSubject :: PEER, TrustQualityArrayGaussian
      {
         {/* LIED             */ Gaussian::ClassicForm(0, 100 / 12.),
          /* BAD_CONNECTION   */ Gaussian::ClassicForm(50, 100 / 2.),
          /* DUPLICATE        */ Gaussian::ClassicForm(80, 100 / 6.),
          /* NEW_INFORMATION  */ Gaussian::ClassicForm(90, 100 / 6.),
          /* NEW_PEER         */ Gaussian::ClassicForm(100, 100 / 6.)
         }
      }
    },
    {
      TrustSubject::TRANSACTION, TrustQualityArrayGaussian
      {
         {/* LIED             */ Gaussian::ClassicForm(0, 100 / 24.),
          /* BAD_CONNECTION   */ Gaussian::ClassicForm(50, 100 / 2.),
          /* DUPLICATE        */ Gaussian::ClassicForm(80, 100 / 6.),
          /* NEW_INFORMATION  */ Gaussian::ClassicForm(90, 100 / 6.),
          /* NEW_PEER         */ Gaussian::ClassicForm(100, 100 / 6.)
         }
      }
    },
    {
      TrustSubject :: OBJECT, TrustQualityArrayGaussian
      {
        {/* LIED             */ Gaussian::ClassicForm(40, 100 / 3.),
         /* BAD_CONNECTION   */ Gaussian::ClassicForm(65, 100 / 2.),
         /* DUPLICATE        */ Gaussian::ClassicForm(70, 100 / 2.),
         /* NEW_INFORMATION  */ Gaussian::ClassicForm(80, 100 / 3.),
         /* NEW_PEER         */ Gaussian::ClassicForm(80, 100 / 2.)
        }
      }
    }
};

}  // namespace p2p
}  // namespace fetch
