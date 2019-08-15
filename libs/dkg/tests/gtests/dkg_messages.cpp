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

#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "dkg/dkg_messages.hpp"

#include "gtest/gtest.h"

#include <string>
#include <vector>

using namespace fetch;
using namespace fetch::dkg;

TEST(dkg_messages, coefficients)
{
  std::vector<std::string> coefficients = {"coeff1"};

  CoefficientsMessage coeff{1, coefficients, "signature"};

  fetch::serializers::MsgPackSerializer serialiser{coeff.Serialize()};

  fetch::serializers::MsgPackSerializer serialiser1(serialiser.data());
  CoefficientsMessage                   coeff1{serialiser1};

  for (uint64_t ii = 0; ii < coeff.coefficients().size(); ++ii)
  {
    EXPECT_EQ(coeff1.coefficients()[ii], coeff.coefficients()[ii]);
  }
  EXPECT_EQ(coeff1.phase(), coeff.phase());
  EXPECT_EQ(coeff1.signature(), coeff.signature());
}

TEST(dkg_messages, shares)
{
  std::unordered_map<DKGMessage::CabinetId, std::pair<std::string, std::string>> shares;
  shares.insert({"0", {"s_ij", "sprime_ij"}});

  SharesMessage shareMessage{1, shares, "signature"};

  fetch::serializers::MsgPackSerializer serialiser{shareMessage.Serialize()};

  fetch::serializers::MsgPackSerializer serialiser1(serialiser.data());
  SharesMessage                         shareMessage1{serialiser1};

  for (auto const &i_share : shareMessage.shares())
  {
    EXPECT_EQ(shareMessage1.shares().find(i_share.first) != shareMessage1.shares().end(), true);
    EXPECT_EQ(i_share.second.first, shareMessage1.shares().at(i_share.first).first);
  }
  EXPECT_EQ(shareMessage1.phase(), shareMessage.phase());
  EXPECT_EQ(shareMessage1.signature(), shareMessage.signature());
}

TEST(dkg_messages, complaints)
{
  std::unordered_set<DKGMessage::CabinetId> complaints;
  ComplaintsMessage                         complaintMessage{complaints, "signature"};

  fetch::serializers::MsgPackSerializer serialiser{complaintMessage.Serialize()};

  fetch::serializers::MsgPackSerializer serialiser1(serialiser.data());
  ComplaintsMessage                     complaintMessage1{serialiser1};

  EXPECT_EQ(complaintMessage1.complaints(), complaintMessage.complaints());
  EXPECT_EQ(complaintMessage1.signature(), complaintMessage.signature());
}

TEST(dkg_messages, envelope)
{
  std::unordered_set<DKGMessage::CabinetId> complaints;
  ComplaintsMessage                         complaintMessage{complaints, "signature"};

  // Put into DKGEnvelope
  DKGEnvelope env{complaintMessage};

  // Serialise the envelope
  fetch::serializers::SizeCounter env_counter;
  env_counter << env;

  fetch::serializers::MsgPackSerializer env_serialiser;
  env_serialiser.Reserve(env_counter.size());
  env_serialiser << env;

  fetch::serializers::MsgPackSerializer env_serialiser1{env_serialiser.data()};
  DKGEnvelope                           env1;
  env_serialiser1 >> env1;

  // Check the message type of envelopes match
  EXPECT_EQ(env1.Message()->type(), DKGMessage::MessageType::COMPLAINT);
  EXPECT_EQ(env1.Message()->signature(), complaintMessage.signature());
  EXPECT_EQ(std::dynamic_pointer_cast<ComplaintsMessage>(env1.Message())->complaints(),
            complaintMessage.complaints());
}
