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

#include "crypto/key_generator.hpp"

#include "gmock/gmock.h"

using namespace fetch;
using namespace fetch::crypto;

class FullConstellationTests : public ::testing::Test
{
  using Certificates          = std::vector<std::shared_ptr<crypto::Prover>>;
  //using Constellations        = std::vector<Constellation>;
  //using ConstellationRunAsync = std::unique_ptr<std::thread>;
protected:

  void SetUp() override
  {
  }

  void TearDown() override
  {
    //ClearAll();
  }

  void StartNodes(uint32_t nodes, uint32_t of_which_are_miners)
  {
    (void)(nodes);
    (void)(of_which_are_miners);

    // Create the identities which the nodes will have
    for (uint32_t i = 0; i < nodes; ++i)
    {
      certificates_.emplace_back(GenerateP2PKey());
    }

    // All nodes must have the same genesis file
    genesis_file_location = CreateGenesisFile(certificates_);
  }

  //DAG          dag_;
  std::string genesis_file_location = "";

private:
  //ProverPtr CreateNewCertificate()
  //{
  //  using Signer    = fetch::crypto::ECDSASigner;
  //  using SignerPtr = std::shared_ptr<Signer>;

  //  SignerPtr certificate = std::make_shared<Signer>();

  //  certificate->GenerateKeys();

  //  return certificate;
  //}
  Certificates certificates_;
};

// Check that the dag can consistently add nodes locally and advance the epochs
TEST_F(FullConstellationTests, CheckBlockGeneration)
{
  StartNodes(10, 10);
  // This function has assertions
  //PopulateDAG();
}

