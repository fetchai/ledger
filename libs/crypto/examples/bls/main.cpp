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

#include "crypto/bls.hpp"

#include <iostream>

using namespace fetch::crypto;

int main()
{
  bls::init();
  std::cout << "Running BLS signature" << std::endl;
  BLSSigner signer;
  signer.GenerateKeys();

  BLSVerifier verifier{signer.identity()};
  auto        sig = signer.Sign("Hello world");

  if (verifier.Verify("Hello world", sig))
  {
    std::cout << "Signature verified." << std::endl;
  }

  return 0;
}
