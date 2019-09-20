#pragma once//------------------------------------------------------------------------------
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
#include "core/service_ids.hpp"
#include "core/state_machine.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/sha256.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"

#include "network/service/protocol.hpp"

#include "moment/clocks.hpp"
#include "moment/deadline_timer.hpp"

#include <map>
#include <set>
#include <random>

namespace fetch {
namespace muddle {

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
    ANSW = 0,
    SIG  = 1,
    SEEN = 2,
  };

  QuestionStruct()                                     = default;
  QuestionStruct(QuestionStruct const &rhs)            = default;
  QuestionStruct(QuestionStruct &&rhs)                 = default;
  QuestionStruct &operator=(QuestionStruct const &rhs) = default;
  QuestionStruct &operator=(QuestionStruct&& rhs)      = default;

  QuestionStruct(Digest const &question, Answer const &answer, CertificatePtr certificate, CabinetMembers const &current_cabinet)
    : certificate_{certificate}
    , self_{certificate_->identity().identifier()}
    , question_{question}
    , cabinet_{current_cabinet}
  {
    // Always populate the table with our answer before anything else
    AnswerAndSeen &cabinet_answer = table_[self_];

    std::get<ANSW>(cabinet_answer)        = answer;
    std::get<SIG>(cabinet_answer)         = Digest("nothing");
    std::get<SEEN>(cabinet_answer)[self_] = Digest("have seen!");

    // Create entries for all desired cabinet members
    for(auto const &member : cabinet_)
    {
      table_[member];
    }

    //FETCH_LOG_INFO(LOGGING_NAME, "Table size: ", );

    // TODO(HUT): signing
  }

  uint64_t UnconfirmedMsgs()
  {
    uint64_t ret = 0;

    for(auto const &entry : table_)
    {
      AnswerAndSeen const &answer_and_seen = entry.second;

      if(!GetSeen(answer_and_seen).empty())
      {
        ret++;
      }
    }

    return ret;
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

  ConfirmedAnswers Update(uint32_t threshold, QuestionStruct &rhs)
  {
    ConfirmedAnswers ret;

    if(rhs.question_ != question_)
    {
      return ret;
    }

    for(auto &entry : table_)
    {
      MuddleAddress const &address         = entry.first;
      AnswerAndSeen &answer_and_seen = entry.second;
      Answer        &answer    = std::get<ANSW>(answer_and_seen);
      Signature     &signature = std::get<SIG>(answer_and_seen);
      Seen          &seen      = std::get<SEEN>(answer_and_seen);
      bool          msg_was_below_threshold = false;

      if(seen.size() < threshold)
      {
        msg_was_below_threshold = true;
      }

      // Add info from other table to ours
      AnswerAndSeen &rhs_answer_and_seen = rhs.table_[address];
      Answer        const &rhs_answer          = GetAnswer(rhs_answer_and_seen);
      Signature     const &rhs_signature       = GetSignature(rhs_answer_and_seen);
      Seen          const &rhs_seen            = GetSeen(rhs_answer_and_seen);

      if(!rhs_answer.empty())
      {
        answer = rhs_answer;

        seen.insert(std::make_pair(self_, Digest("temp")));
      }

      if(!rhs_signature.empty())
      {
        signature = rhs_signature;
      }

      for(auto const &rhs_seen_pair : rhs_seen)
      {
        seen.insert(rhs_seen_pair);
      }

      if(seen.size() >= threshold && msg_was_below_threshold && address != self_)
      {
        ret.emplace_back(std::make_pair(address, answer));
      }
    }

    return ret;
  }

  // Considered invalid if there is no cabinet
  operator bool() const
  {
    return !cabinet_.empty();
  }

  CertificatePtr       certificate_;
  MuddleAddress        self_;
  Digest               question_;
  SyncTable            table_;
  CabinetMembers       cabinet_;
};

/**
 */
class PunishmentBroadcastChannel : public service::Protocol, public BroadcastChannelInterface
{
public:

  enum class State
  {
    INIT,
    RESOLVE_PROMISES,
  };

  using CertificatePtr = std::shared_ptr<fetch::crypto::Prover>;
  using Endpoint        = muddle::MuddleEndpoint;
  using ConstByteArray  = byte_array::ConstByteArray;
  using MuddleAddress   = ConstByteArray;
  using CabinetMembers  = std::set<MuddleAddress>;
  using CabinetMembersVector  = std::vector<MuddleAddress>;
  using Subscription    = muddle::Subscription;
  using SubscriptionPtr = std::shared_ptr<muddle::Subscription>;
  using HashFunction    = crypto::SHA256;
  using HashDigest      = byte_array::ByteArray;
  using CallbackFunction =
      std::function<void(MuddleAddress const &, byte_array::ConstByteArray const &)>;
  using Server                  = fetch::muddle::rpc::Server;
  using ServerPtr               = std::shared_ptr<Server>;
  using StateMachine            = core::StateMachine<State>;
  using StateMachinePtr         = std::shared_ptr<StateMachine>;

  // The only RPC function exposed
  enum
  {
    PULL_INFO_FROM_PEER = 1
  };

  QuestionStruct AllowPeerPull()
  {
    FETCH_LOCK(lock_);
    return question_;
  }

  PunishmentBroadcastChannel(Endpoint &endpoint, MuddleAddress address, CallbackFunction call_back,
      CertificatePtr certificate, uint16_t channel = CHANNEL_RBC_BROADCAST, bool ordered_delivery = true)
    : endpoint_{endpoint}
    , address_{address}
    , deliver_msg_callback_{call_back}
    , channel_{channel}
    , certificate_{certificate}
    , rpc_client_{"PunishmentBC", endpoint_, SERVICE_PBC, channel_}
    , state_machine_{std::make_shared<StateMachine>("PBCStateMach", State::INIT)}
  {
    FETCH_UNUSED(ordered_delivery);
    this->Expose(PULL_INFO_FROM_PEER, this, &PunishmentBroadcastChannel::AllowPeerPull);

    // TODO(HUT): rpc beacon rename.
    // Attaching the protocol
    rpc_server_ = std::make_shared<Server>(endpoint_, SERVICE_PBC, channel_);
    rpc_server_->Add(RPC_BEACON, this);

    // Connect states to the state machine
    state_machine_->RegisterHandler(State::INIT, this,
                                  &PunishmentBroadcastChannel::OnInit);
    state_machine_->RegisterHandler(State::RESOLVE_PROMISES, this,
                                  &PunishmentBroadcastChannel::OnResolvePromises);
  }

  // Interface methods
  bool ResetCabinet(CabinetMembers const &cabinet) override
  {
    FETCH_LOCK(lock_);
    current_cabinet_ = cabinet;

    // Set threshold depending on size of committee
    if (current_cabinet_.size() % 3 == 0)
    {
      threshold_ = static_cast<uint32_t>(current_cabinet_.size() / 3 - 1);
    }
    else
    {
      threshold_ = static_cast<uint32_t>(current_cabinet_.size() / 3);
    }

    assert(current_cabinet_.size() > 3 * threshold_);

    return true;
  }

  void SetQuestion(ConstByteArray const &question, ConstByteArray const &answer) override
  {
    FETCH_LOCK(lock_);
    previous_question_ = std::move(question_);
    question_ = QuestionStruct(question, answer, certificate_, current_cabinet_);

    if(!question_)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "bad things");
    }
  }

  void SetQuestion(std::string const &question, std::string const &answer) /* override */
  {
    SetQuestion(ConstByteArray(question), ConstByteArray(answer));
  }

  void Enable(bool enable) override
  {
    FETCH_LOCK(lock_);
    enabled_ = enable;
  }

  std::weak_ptr<core::Runnable> GetRunnable() override
  {
    return state_machine_;
  }

  /// @}

  bool AnsweredQuestion()
  {
    uint32_t messages_confirmed = 0;

    for(auto const &node_plus_answers : question_.table_)
    {
      if(QuestionStruct::GetSeen(node_plus_answers.second).size() >= threshold_ && node_plus_answers.first != question_.self_)
      {
        messages_confirmed++;
      }
    }

    bool const answered_question = messages_confirmed == question_.cabinet_.size() - 1;

    //FETCH_LOG_INFO(LOGGING_NAME, "Answered qn: ", answered_question, " confirmed: ", messages_confirmed);

    return answered_question;
  }

  PunishmentBroadcastChannel::State OnInit()
  {
    {
      FETCH_LOCK(lock_);

      if(!question_ || !enabled_ || AnsweredQuestion())
      {
        //FETCH_LOG_INFO(LOGGING_NAME, "Held at idle: ", question_, enabled_, AnsweredQuestion(), " thre: ", threshold_);
        state_machine_->Delay(std::chrono::milliseconds(50));
        network_promises_.clear();
        return State::INIT;
      }

      if(current_cabinet_vector_.size() < concurrent_promises_allowed)
      {
        current_cabinet_vector_.clear();

        for(auto const &member : current_cabinet_)
        {
          current_cabinet_vector_.push_back(member);
        }

        // Make N requests for the other peer's answer
        std::random_device rd;
        std::mt19937       g(rd());
        std::shuffle(current_cabinet_vector_.begin(), current_cabinet_vector_.end(), g);
      }
    }

    for (std::size_t i = 0; i < concurrent_promises_allowed; ++i)
    {
      MuddleAddress send_to = current_cabinet_vector_.back();
      current_cabinet_vector_.pop_back();

      auto promise     = rpc_client_.CallSpecificAddress(
          send_to, RPC_BEACON, PunishmentBroadcastChannel::PULL_INFO_FROM_PEER);

      network_promises_.emplace_back(std::make_pair(send_to, promise));
    }

    time_to_wait_.Restart(REASONABLE_NETWORK_DELAY_MS);
    return State::RESOLVE_PROMISES;
  }

  PunishmentBroadcastChannel::State OnResolvePromises()
  {
    for (auto it = network_promises_.begin(); it != network_promises_.end();)
    {
      MuddleAddress &address = (*it).first;
      auto &promise          = (*it).second;

      if(promise->IsSuccessful())
      {
        QuestionStruct recvd_question;

        if(!promise->As<QuestionStruct>(recvd_question))
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Failed to deserialize response from: ", address.ToBase64());
        }
        else
        {
          QuestionStruct::ConfirmedAnswers answers;

          {
            FETCH_LOCK(lock_);
            //FETCH_LOG_INFO(LOGGING_NAME, "Has messages: ", recvd_question.UnconfirmedMsgs(), " We have: ", question_.UnconfirmedMsgs());

            answers = question_.Update(threshold_, recvd_question);
          }


          for(auto const &answer : answers)
          {
            //FETCH_LOG_INFO(LOGGING_NAME, "Delivering messages!");
            deliver_msg_callback_(answer.first, answer.second);
          }
        }

        it = network_promises_.erase(it);
      }
      else
      {
        ++it;
      }
    }

    if(network_promises_.empty() || time_to_wait_.HasExpired())
    {
      if(!network_promises_.empty())
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to resolve promises: ", network_promises_.size());
      }

      return State::INIT;
    }

    state_machine_->Delay(std::chrono::milliseconds(50));
    return State::RESOLVE_PROMISES;
  }

  // TODO(HUT): use mutable aspect
  mutable Mutex lock_;

  QuestionStruct question_;
  QuestionStruct previous_question_;

  // don't need to lock these.
  Endpoint &          endpoint_;              ///< The muddle endpoint to communicate on
  MuddleAddress const address_;               ///< Our muddle address
  CallbackFunction    deliver_msg_callback_;  ///< Callback for messages which have succeeded
  uint16_t            channel_;
  CertificatePtr      certificate_; // TODO(HUT): check if can be const

  ServerPtr           rpc_server_{nullptr};
  muddle::rpc::Client rpc_client_;

  bool enabled_ = true;

  // For broadcast
  CabinetMembers            current_cabinet_;    ///< The set of muddle addresses of the
                                           ///< cabinet (including our own)
  CabinetMembersVector      current_cabinet_vector_;    ///< The set of muddle addresses of the
  uint32_t threshold_;                     ///< Number of byzantine nodes (this is assumed
                                           ///< to take the maximum allowed value satisying
                                           ///< threshold_ < current_cabinet_.size()
                                           //


  moment::ClockPtr clock_ = moment::GetClock("muddle:pbc", moment::ClockType::STEADY);
  moment::DeadlineTimer    time_to_wait_{"muddle:pbc"};

  const uint8_t concurrent_promises_allowed = 2;
  const uint16_t REASONABLE_NETWORK_DELAY_MS = 500;
  std::vector<std::pair<MuddleAddress, service::Promise>> network_promises_;

  /// @}
  StateMachinePtr    state_machine_;
};
}  // namespace muddle

namespace serializers {

template <typename D>
struct MapSerializer<muddle::QuestionStruct, D>
{
public:
  using Type       = muddle::QuestionStruct;
  using DriverType = D;

  static uint8_t const TABLE            = 1;
  static uint8_t const QUESTION         = 2;
  static uint8_t const CABINET          = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &table)
  {
    auto map = map_constructor(3);
    map.Append(TABLE, table.table_);
    map.Append(QUESTION, table.question_);
    map.Append(CABINET, table.cabinet_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &table)
  {
    map.ExpectKeyGetValue(TABLE, table.table_);
    map.ExpectKeyGetValue(QUESTION, table.question_);
    map.ExpectKeyGetValue(CABINET, table.cabinet_);
  }
};

template <typename D>
struct MapSerializer<muddle::QuestionStruct::AnswerAndSeen, D>
{
public:
  using Type       = muddle::QuestionStruct::AnswerAndSeen;
  using DriverType = D;

  static uint8_t const ANSWER    = 1;
  static uint8_t const SIGNATURE = 2;
  static uint8_t const SEEN      = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &tuple)
  {
    auto map = map_constructor(3);
    map.Append(ANSWER,    std::get<0>(tuple));
    map.Append(SIGNATURE, std::get<1>(tuple));
    map.Append(SEEN,      std::get<2>(tuple));
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &tuple)
  {
    map.ExpectKeyGetValue(ANSWER,    std::get<0>(tuple));
    map.ExpectKeyGetValue(SIGNATURE, std::get<1>(tuple));
    map.ExpectKeyGetValue(SEEN,  std::get<2>(tuple));
  }
};

}  // namespace serializers

}  // namespace fetch

