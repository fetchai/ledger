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

#include "beacon/dkg_output.hpp"

using fetch::beacon::DkgOutput;

DkgOutput::DkgOutput()
{
  fetch::crypto::mcl::details::MCLInitialiser();
  group_public_key.clear();
  private_key_share.clear();
}

DkgOutput::DkgOutput(PublicKey group_key, std::vector<PublicKey> key_shares,
                     PrivateKey  secret_share,  // NOLINT
                     CabinetList qual_members)
  : qual{std::move(qual_members)}
  , group_public_key{std::move(group_key)}
  , public_key_shares{std::move(key_shares)}
  , private_key_share{secret_share}
{}

DkgOutput::DkgOutput(DkgKeyInformation const &keys, CabinetList qual_members)
  : DkgOutput{keys.group_public_key, keys.public_key_shares, keys.private_key_share,
              std::move(qual_members)}
{}
