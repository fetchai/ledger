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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ledger/protocols/dag_service.hpp"
#include "ledger/dag/dag_interface.hpp"
#include "../../../network/tests/muddle/fake_muddle_endpoint.hpp"

#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::StrictMock;

using namespace fetch;

static constexpr char const *LOGGING_NAME = "DagServiceTests";

using DAGService         = ledger::DAGService;
using DAGServiceModes    = DAGService::Mode;
using FakeMuddleEndpoint = muddle::FakeMuddleEndpoint;

using DAGChild        = ledger::DAG;
using DAGInterface    = ledger::DAGInterface;
using DAGServices     = std::vector<std::shared_ptr<DAGService>>;
using DAGs            = std::vector<std::shared_ptr<DAGInterface>>;

// Muddle endpoint is faked
using FakeMuddleEndpoint = muddle::FakeMuddleEndpoint;
using MuddleEndpoints    = std::vector<FakeMuddleEndpoint>;

using Prover         = fetch::crypto::Prover;
using ProverPtr      = std::shared_ptr<Prover>;
using Certificate       = fetch::crypto::Prover;
using CertificatePtr    = std::shared_ptr<Certificate>;

class DagServiceTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    FETCH_UNUSED(LOGGING_NAME);

    // At least one DAG service per test
    AddDAGService();
  }

  void TearDown() override
  {
  }

  void AddDAGService()
  {
    // Create a muddle endpoint for the service
    muddle_endpoints_.push_back(FakeMuddleEndpoint());

    // A Dag
    auto certificate = CreateNewCertificate();
    std::shared_ptr<DAGInterface> dag_interface;
    dag_interface.reset(new DAGChild("TODO(HUT): ", true, certificate));
    dags_.push_back(dag_interface);

    // The service
    DAGService serv(muddle_endpoints_.back(), dag_interface);
    //dag_services_.push_back(std::make_shared<DAGService>(&muddle_endpoints_.back(), dag_interface)); // TODO(HUT): mode
  }

  DAGServiceModes  mode_{DAGServiceModes::CREATE_DATABASE};

  DAGServices      dag_services_;
  DAGs             dags_;
  MuddleEndpoints  muddle_endpoints_;

private:

  ProverPtr CreateNewCertificate()
  {
    using Signer    = fetch::crypto::ECDSASigner;
    using SignerPtr = std::shared_ptr<Signer>;

    SignerPtr certificate        = std::make_shared<Signer>();

    certificate->GenerateKeys();

    return certificate;
  }
};

//TEST_F(DagServiceTests, CheckBasicDagFunctionality)
//{
//}
