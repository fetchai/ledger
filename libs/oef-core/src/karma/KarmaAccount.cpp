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

#include "oef-core/karma/KarmaAccount.hpp"

#include "logging/logging.hpp"
#include "oef-core/karma/IKarmaPolicy.hpp"

void swap(KarmaAccount &v1, KarmaAccount &v2)
{
  v1.swap(v2);
}

bool KarmaAccount::perform(const std::string &action, bool /*force*/)
{
  if (policy == nullptr)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Denying karma for '", action, "' because account is VOID");
    return false;
  }
  auto r = policy->perform(*this, action);
  if (!r)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Denying karma for '", action, "' because balance is too low.");
  }
  return r;
}

bool KarmaAccount::CouldPerform(std::string const &action)
{
  if (policy == nullptr)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Karma for '", action,
                   "' would be denied because account is VOID");
    return false;
  }
  auto r = policy->CouldPerform(*this, action);
  if (!r)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Karma for '", action,
                   "' would be denied because balance is too low.");
  }
  return r;
}

void KarmaAccount::upgrade(const std::string &pubkey, const std::string &ip)
{
  this->policy->upgrade(*this, pubkey, ip);
}

std::string KarmaAccount::GetBalance()
{
  return policy->GetBalance(*this);
}
