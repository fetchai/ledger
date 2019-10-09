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
#include "crypto/prover.hpp"

#include <map>
#include <set>
#include <vector>

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

  QuestionStruct();

  QuestionStruct(Digest question, Answer answer, CertificatePtr certificate,
                 CabinetMembers current_cabinet);

  static Answer const &   GetAnswer(AnswerAndSeen const &ref);
  static Signature const &GetSignature(AnswerAndSeen const &ref);
  static Seen const &     GetSeen(AnswerAndSeen const &ref);

  /**
   * Update entries in our own table with some other table, if
   * the signatures, question etc. are correct.
   *
   * Return the answers which pass the threshold due to this
   * (one time event)
   */
  ConfirmedAnswers Update(uint32_t threshold, QuestionStruct &rhs);

  // Considered invalid if there is no cabinet
  explicit operator bool() const;

  CertificatePtr certificate_;  // Our certificate
  MuddleAddress  self_;         // Our address
  Digest         question_;     // The question hash
  SyncTable      table_;        // The table to populate
  CabinetMembers cabinet_;      // the cabinet
};

}  // namespace muddle
}  // namespace fetch
