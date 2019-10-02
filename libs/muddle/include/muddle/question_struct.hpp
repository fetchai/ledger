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

#include "core/byte_array/const_byte_array.hpp"

#include <map>
#include <set>

namespace fetch {
namespace muddle {

/**
 * Structure used for the punishment broadcast channel.
 *
 */
struct QuestionStruct
{
  using CertificatePtr   = std::shared_ptr<fetch::crypto::Prover>;
  using MuddleAddress    = byte_array::ConstByteArray;
  using CabinetMembers   = std::set<MuddleAddress>;
  using Digest           = byte_array::ConstByteArray;
  using Answer           = byte_array::ConstByteArray;
  using Signature        = byte_array::ConstByteArray;
  using SeenProof        = byte_array::ConstByteArray;
  using Seen             = std::map<MuddleAddress, SeenProof>;
  using AnswerAndSeen    = std::tuple<Answer, Signature, Seen>;
  using SyncTable        = std::map<MuddleAddress, AnswerAndSeen>;
  using ConfirmedAnswers = std::vector<std::pair<MuddleAddress, Answer>>;

  static constexpr const char *LOGGING_NAME = "QuestionStruct";

  enum : int
  {
    ANSW = 0,  // Our answer
    SIG  = 1,  // Signature of answer
    SEEN = 2,  // Signature indicating seen
  };

  QuestionStruct() = default;

  QuestionStruct(Digest question, Answer answer, CertificatePtr certificate,
                 CabinetMembers current_cabinet)
    : certificate_{std::move(certificate)}
    , self_{certificate_->identity().identifier()}
    , question_{std::move(question)}
    , cabinet_{std::move(current_cabinet)}
  {
    // Always populate the table with our answer before anything else
    AnswerAndSeen &cabinet_answer = table_[self_];

    std::get<ANSW>(cabinet_answer)        = std::move(answer);
    std::get<SIG>(cabinet_answer)         = Digest("nothing");
    std::get<SEEN>(cabinet_answer)[self_] = Digest("have seen!");

    // Always create entries for all desired cabinet members to avoid
    // indexing errors
    for (auto const &member : cabinet_)
    {
      table_[member];
    }
    // TODO(HUT): signing/confirmation
  }

  static Answer const &GetAnswer(AnswerAndSeen const &ref)
  {
    return std::get<ANSW>(ref);
  }

  static Signature const &GetSignature(AnswerAndSeen const &ref)
  {
    return std::get<SIG>(ref);
  }

  static Seen const &GetSeen(AnswerAndSeen const &ref)
  {
    return std::get<SEEN>(ref);
  }

  /**
   * Update entries in our own table with some other table, if
   * the signatures, question etc. are correct.
   *
   * Return the answers which pass the threshold due to this
   * (one time event)
   */
  ConfirmedAnswers Update(uint32_t threshold, QuestionStruct &rhs)
  {
    ConfirmedAnswers ret;

    if (rhs.question_ != question_)
    {
      return ret;
    }

    for (auto &entry : table_)
    {
      MuddleAddress const &address                 = entry.first;
      AnswerAndSeen &      answer_and_seen         = entry.second;
      Answer &             answer                  = std::get<ANSW>(answer_and_seen);
      Signature &          signature               = std::get<SIG>(answer_and_seen);
      Seen &               seen                    = std::get<SEEN>(answer_and_seen);
      bool                 msg_was_below_threshold = false;

      if (seen.size() < threshold)
      {
        msg_was_below_threshold = true;
      }

      // Add info from other table to ours
      AnswerAndSeen &  rhs_answer_and_seen = rhs.table_[address];
      Answer const &   rhs_answer          = GetAnswer(rhs_answer_and_seen);
      Signature const &rhs_signature       = GetSignature(rhs_answer_and_seen);
      Seen const &     rhs_seen            = GetSeen(rhs_answer_and_seen);

      if (!rhs_answer.empty() && answer.empty())
      {
        answer = rhs_answer;

        seen.insert(std::make_pair(self_, Digest("temp")));
      }

      if (!rhs_signature.empty() && signature.empty())
      {
        signature = rhs_signature;
      }

      for (auto const &rhs_seen_pair : rhs_seen)
      {
        seen.insert(rhs_seen_pair);
      }

      if (seen.size() >= threshold && msg_was_below_threshold && address != self_)
      {
        ret.emplace_back(std::make_pair(address, answer));
      }
    }

    return ret;
  }

  // Considered invalid if there is no cabinet
  operator bool() const  // NOLINT
  {
    return !cabinet_.empty();
  }

  CertificatePtr certificate_;  // Our certificate
  MuddleAddress  self_;         // Our address
  Digest         question_;     // The question hash
  SyncTable      table_;        // The table to populate
  CabinetMembers cabinet_;      // the cabinet
};

}  // namespace muddle
}  // namespace fetch
