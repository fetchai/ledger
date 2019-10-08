#pragma once
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

#include "crypto/mcl_dkg.hpp"

namespace fetch {
namespace beacon {

struct DkgOutput
{
  using PublicKey         = crypto::mcl::PublicKey;
  using PrivateKey        = crypto::mcl::PrivateKey;
  using DkgKeyInformation = crypto::mcl::DkgKeyInformation;
  using MuddleAddress     = byte_array::ConstByteArray;
  using CabinetList       = std::set<MuddleAddress>;

  DkgOutput();

  DkgOutput(PublicKey group_key, std::vector<PublicKey> key_shares, PrivateKey const &secret_share,
            CabinetList qual_members);

  DkgOutput(DkgKeyInformation const &keys, CabinetList qual_members);

  CabinetList            qual{};
  PublicKey              group_public_key;
  std::vector<PublicKey> public_key_shares{};
  PrivateKey             private_key_share;
};

}  // namespace beacon
}  // namespace fetch
