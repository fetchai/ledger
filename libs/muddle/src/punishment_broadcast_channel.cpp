//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "muddle/punishment_broadcast_channel.hpp"

using fetch::muddle::PunishmentBroadcastChannel;
using fetch::muddle::QuestionStruct;

QuestionStruct PunishmentBroadcastChannel::AllowPeerPull()
{
  FETCH_LOCK(lock_);
  return question_;
}

PunishmentBroadcastChannel::PunishmentBroadcastChannel(Endpoint &endpoint, MuddleAddress address,
                                                       CallbackFunction call_back,
                                                       CertificatePtr certificate, uint16_t channel,
                                                       bool ordered_delivery)
  : endpoint_{endpoint}
  , address_{std::move(address)}
  , deliver_msg_callback_{std::move(call_back)}
  , channel_{channel}
  , certificate_{std::move(certificate)}
  , rpc_client_{"PunishmentBC", endpoint_, SERVICE_PBC, channel_}
  , state_machine_{std::make_shared<StateMachine>("PBCStateMach", State::INIT)}
{
  FETCH_UNUSED(ordered_delivery);
  Expose(PULL_INFO_FROM_PEER, this, &PunishmentBroadcastChannel::AllowPeerPull);

  // TODO(HUT): rpc beacon rename.
  // Attaching the protocol
  rpc_server_ = std::make_shared<Server>(endpoint_, SERVICE_PBC, channel_);
  rpc_server_->Add(RPC_BEACON, this);

  // Connect states to the state machine
  state_machine_->RegisterHandler(State::INIT, this, &PunishmentBroadcastChannel::OnInit);
  state_machine_->RegisterHandler(State::RESOLVE_PROMISES, this,
                                  &PunishmentBroadcastChannel::OnResolvePromises);
}

PunishmentBroadcastChannel::~PunishmentBroadcastChannel() = default;
//{
//  // TODO(HUT): reinstate this fix.
//  /* rpc_server_->Remove(RPC_BEACON); */
//}

// Interface methods
bool PunishmentBroadcastChannel::ResetCabinet(CabinetMembers const &cabinet)
{
  FETCH_LOCK(lock_);
  current_cabinet_ = cabinet;

  // Set threshold depending on size of cabinet
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

/**
 * Set/reset the question, saving the old question, so that peer who have not yet
 * reset can still access this information
 *
 */
void PunishmentBroadcastChannel::SetQuestion(ConstByteArray const &question,
                                             ConstByteArray const &answer)
{
  FETCH_LOCK(lock_);
  previous_question_ = std::move(question_);
  question_          = QuestionStruct(question, answer, certificate_, current_cabinet_);
}

void PunishmentBroadcastChannel::SetQuestion(std::string const &question, std::string const &answer)
{
  SetQuestion(ConstByteArray(question), ConstByteArray(answer));
}

void PunishmentBroadcastChannel::Enable(bool enable)
{
  FETCH_LOCK(lock_);
  enabled_ = enable;
}

/**
 * The channel needs to be driven to resolve network events
 */
std::weak_ptr<fetch::core::Runnable> PunishmentBroadcastChannel::GetRunnable()
{
  return state_machine_;
}

/**
 * Determine whether the current question has been answered by all peers.
 * If not, continue to try to resolve answers
 */
bool PunishmentBroadcastChannel::AnsweredQuestion()
{
  uint32_t messages_confirmed = 0;

  for (auto const &node_plus_answers : question_.table_)
  {
    if (QuestionStruct::GetSeen(node_plus_answers.second).size() >= threshold_ &&
        node_plus_answers.first != question_.self_)
    {
      messages_confirmed++;
    }
  }

  bool const answered_question = messages_confirmed == question_.cabinet_.size() - 1;

  return answered_question;
}

PunishmentBroadcastChannel::State PunishmentBroadcastChannel::OnInit()
{
  // Determine whether to take action
  {
    FETCH_LOCK(lock_);

    if (!question_ || !enabled_ || AnsweredQuestion())
    {
      state_machine_->Delay(std::chrono::milliseconds(50));
      network_promises_.clear();
      return State::INIT;
    }
  }

  // If so, populate a vector with random peer addresses to try
  // (up to concurrent_promises_allowed at once)
  if (current_cabinet_vector_.size() < concurrent_promises_allowed)
  {
    current_cabinet_vector_.clear();

    for (auto const &member : current_cabinet_)
    {
      if (member != certificate_->identity().identifier())
      {
        current_cabinet_vector_.push_back(member);
      }
    }

    std::random_device rd;
    std::mt19937       g(rd());
    std::shuffle(current_cabinet_vector_.begin(), current_cabinet_vector_.end(), g);
  }

  // Dispatch requests for peers tables and set timer
  for (std::size_t i = 0; i < concurrent_promises_allowed; ++i)
  {
    MuddleAddress send_to = current_cabinet_vector_.back();
    current_cabinet_vector_.pop_back();

    auto promise = rpc_client_.CallSpecificAddress(send_to, RPC_BEACON,
                                                   PunishmentBroadcastChannel::PULL_INFO_FROM_PEER);

    network_promises_.emplace_back(std::make_pair(send_to, promise));
  }

  time_to_wait_.Restart(REASONABLE_NETWORK_DELAY_MS);
  return State::RESOLVE_PROMISES;
}

/**
 * Attempt to resolve the network promises and add them to our table,
 * eventually timing out.
 */
PunishmentBroadcastChannel::State PunishmentBroadcastChannel::OnResolvePromises()
{
  for (auto it = network_promises_.begin(); it != network_promises_.end();)
  {
    MuddleAddress &address = it->first;
    auto &         promise = it->second;

    if (promise->IsSuccessful())
    {
      QuestionStruct recvd_question;

      if (!promise->GetResult(recvd_question))
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to deserialize response from: ", address.ToBase64());
      }
      else
      {
        QuestionStruct::ConfirmedAnswers answers;

        {
          FETCH_LOCK(lock_);

          // Guard against receiving a non-matching table
          if (recvd_question.question_ != question_.question_)
          {
            FETCH_LOG_DEBUG(LOGGING_NAME, "Note: ignoring non matching question");
          }
          else
          {
            answers = question_.Update(threshold_, recvd_question);
          }
        }

        for (auto const &answer : answers)
        {
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

  if (network_promises_.empty() || time_to_wait_.HasExpired())
  {
    if (!network_promises_.empty())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to resolve promises: ", network_promises_.size());

      network_promises_.clear();
    }

    return State::INIT;
  }

  state_machine_->Delay(std::chrono::milliseconds(50));
  return State::RESOLVE_PROMISES;
}
