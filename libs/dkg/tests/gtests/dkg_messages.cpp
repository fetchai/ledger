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

#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/counter.hpp"
#include "dkg/dkg_messages.hpp"
#include "gtest/gtest.h"

using namespace fetch;
using namespace fetch::dkg;

TEST(dkg_messages, coefficients)
{
  std::vector<std::string> coefficients;
  coefficients.push_back("coeff1");
  CoefficientsMessage coeff{1, coefficients, "signature"};

  fetch::serializers::ByteArrayBuffer serialiser{coeff.Serialize()};

  fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
  CoefficientsMessage                 coeff1{serialiser1};

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

  fetch::serializers::ByteArrayBuffer serialiser{shareMessage.Serialize()};

  fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
  SharesMessage                       shareMessage1{serialiser1};

  for (const auto &i_share : shareMessage.shares())
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
  ComplaintsMessage                         complaintMsg{complaints, "signature"};

  fetch::serializers::ByteArrayBuffer serialiser{complaintMsg.Serialize()};

  fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
  ComplaintsMessage                   complaintMsg1{serialiser1};

  EXPECT_EQ(complaintMsg1.complaints(), complaintMsg.complaints());
  EXPECT_EQ(complaintMsg1.signature(), complaintMsg.signature());
}

TEST(dkg_messages, envelope)
{
  std::unordered_set<DKGMessage::CabinetId> complaints;
  ComplaintsMessage                         complaintMsg{complaints, "signature"};

  // Put into DKGEnvelop
  DKGEnvelop env{complaintMsg};

  // Serialise the envelop
  fetch::serializers::SizeCounter<fetch::serializers::ByteArrayBuffer> env_counter;
  env_counter << env;

  fetch::serializers::ByteArrayBuffer env_serialiser;
  env_serialiser.Reserve(env_counter.size());
  env_serialiser << env;

  fetch::serializers::ByteArrayBuffer env_serialiser1{env_serialiser.data()};
  DKGEnvelop                          env1;
  env_serialiser1 >> env1;

  // Check the message type of envelops match
  EXPECT_EQ(env1.Message()->type(), DKGMessage::MessageType::COMPLAINT);
  EXPECT_EQ(env1.Message()->signature(), complaintMsg.signature());
  EXPECT_EQ(std::dynamic_pointer_cast<ComplaintsMessage>(env1.Message())->complaints(),
            complaintMsg.complaints());
}
