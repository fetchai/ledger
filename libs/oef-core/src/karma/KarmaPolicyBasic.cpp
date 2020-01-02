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

#include "logging/logging.hpp"
#include "oef-core/karma/KarmaPolicyBasic.hpp"
#include "oef-core/karma/XDisconnect.hpp"
#include "oef-core/karma/XKarma.hpp"

std::atomic<std::size_t> tick_amounts(0);
std::atomic<std::size_t> tick_counter(0);

static const std::size_t MAX_KARMA = 10000;

KarmaPolicyBasic::Account::Account()
  : karma(MAX_KARMA)
  , when(0)
{}

void KarmaPolicyBasic::Account::BringUpToDate()
{
  while (true)
  {
    KARMA old_karma = karma;
    TICKS tc        = tick_counter;
    TICKS diff      = tc - when;
    KARMA new_karma = static_cast<KARMA>(
        std::min(static_cast<unsigned long>(karma) + diff * tick_amounts, MAX_KARMA));

    // Write this as long as nothing has updated karma while we were thinking.
    if (karma.compare_exchange_strong(old_karma, new_karma, std::memory_order_seq_cst))
    {
      FETCH_LOG_INFO(LOGGING_NAME, "KARMA: Bring up to date: ticks=", diff, " step=", tick_amounts,
                     " when=", when, " now=", tc, " old=", old_karma, " new=", new_karma);
      when = tc;
      break;
    }
  }
}

std::string KarmaPolicyBasic::GetBalance(const KarmaAccount &identifier)
{
  return std::string("account=") + identifier.GetName() +
         "  karma=" + std::to_string(accounts.access(*identifier).karma) + "/" +
         std::to_string(MAX_KARMA);
}

KarmaPolicyBasic::KarmaPolicyBasic(const google::protobuf::Map<std::string, std::string> &config)
{
  accounts.get("(null karma account)");  // make sure the null account exists.

  for (auto &kv : config)
  {
    this->config[kv.first] = kv.second;
  }
}

void KarmaPolicyBasic::RefreshCycle(const std::chrono::milliseconds /*delta*/)
{
  auto policies = getPolicies("refresh");
  tick_amounts  = static_cast<TICKS>(parseEffect(0, policies[0]));
  // FETCH_LOG_INFO(LOGGING_NAME, "KARMA: refreshTick of size ", tick_amounts);
  tick_counter++;
}

std::size_t KarmaPolicyBasic::GetAccountNumber(const std::string &s)
{
  return accounts.get(s);
}

KarmaAccount KarmaPolicyBasic::GetAccount(const std::string &pubkey, const std::string &ip)
{
  if (pubkey.length() > 0)
  {
    return mkAccount(GetAccountNumber(pubkey), pubkey);
  }
  else if (ip.length() > 0)
  {
    return mkAccount(GetAccountNumber(ip), ip);
  }
  else
  {
    return mkAccount(0, "DEFAULT_KARMA_ACCOUNT");
  }
}

void KarmaPolicyBasic::upgrade(KarmaAccount &account, const std::string &pubkey,
                               const std::string &ip)
{
  auto k = GetAccount(pubkey, ip);
  std::swap(account, k);
}

KarmaPolicyBasic::Account &KarmaPolicyBasic::access(const KarmaAccount &identifier)
{
  return accounts.access(*identifier);
}

const KarmaPolicyBasic::Account &KarmaPolicyBasic::access(const KarmaAccount &identifier) const
{
  return accounts.access(*identifier);
}

std::string zero("0");

void tokenise(const std::string &input, std::vector<std::string> &output, const char delim)
{
  std::size_t start = 0;
  std::size_t end   = 0;

  while (true)
  {
    start = input.find_first_not_of(delim, end);
    if (start == std::string::npos)
    {
      return;
    }
    end = input.find(delim, start);  // could be npos.
    output.push_back(input.substr(
        start, end - start));  // npos - start is still a huge number, this gets the last token.
  }
}

std::vector<std::string> KarmaPolicyBasic::getPolicies(const std::string &action) const
{
  std::vector<std::string> actions;
  tokenise(action, actions, ',');
  if (actions.empty())
  {
    actions.emplace_back("*");
  }

  std::vector<std::string> policies;
  for (auto const &a : actions)
  {
    policies.push_back(getPolicy(a));
  }

  return policies;
}

std::string KarmaPolicyBasic::getLessSpecificAction(const std::string &action) const
{
  if (action == "*")
  {
    return action;
  }
  auto r = action.find_last_of('.');
  if (r == std::string::npos || r == 0)
  {
    return "*";
  }
  return action.substr(0, r - 1);
}

const std::string &KarmaPolicyBasic::getPolicy(const std::string &action) const
{
  Config::const_iterator iter;
  if ((iter = config.find(action)) != config.end())
  {
    // FETCH_LOG_INFO(LOGGING_NAME, "KARMA: Karma Event:", action, " => ", iter -> second);
    return iter->second;
  }
  if ((iter = config.find("*")) != config.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "KARMA:  Unknown Karma Event:", action);
    return iter->second;
  }
  FETCH_LOG_INFO(LOGGING_NAME, "KARMA:  No default karma event:", action);
  return zero;
}

KarmaPolicyBasic::KARMA KarmaPolicyBasic::afterwards(KARMA              currentBalance,
                                                     const std::string &actions)
{
  auto policies = getPolicies(actions);
  auto worst    = currentBalance;

  try
  {
    for (const auto &policy : policies)
    {
      auto result = parseEffect(currentBalance, policy);
      FETCH_LOG_INFO(LOGGING_NAME, "KARMA: afterwards ", policy, "    ", currentBalance, " => ",
                     result);
      if (result < worst)
      {
        worst = result;
      }
    }
  }
  catch (XKarma const &x)
  {
    throw XKarma(std::string("actions:") + actions + " result in disconnect due to Karma policy");
  }

  return worst;
}

KarmaPolicyBasic::KARMA KarmaPolicyBasic::parseEffect(KARMA              currentBalance,
                                                      const std::string &effect)
{
  switch (effect[0])
  {
  case 'X':
    throw XDisconnect("Disconnect due to Karma policy");
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
  case '+':
  case '-':
  {
    KARMA delta = std::stoi(effect);
    return currentBalance + delta;
  }
  case '=':
    return std::stoi(effect.substr(1));
  default:
  {
    FETCH_LOG_INFO(LOGGING_NAME, "KARMA: Effect which can't be parsed:", effect);
    return currentBalance;
  }
  };
}

bool KarmaPolicyBasic::perform(const KarmaAccount &identifier, const std::string &event, bool force)
{
  accounts.access(*identifier).BringUpToDate();
  KARMA prev = accounts.access(*identifier).karma;
  KARMA next = afterwards(prev, event);

  if (next >= 0 || force)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "KARMA: Event ", event, " for ", identifier.GetName(),
                   " karma change ", prev, " => ", next);
    accounts.access(*identifier).karma = next;
    return true;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "KARMA: Event ", event, " for ", identifier.GetName(),
                 " rejected because result karma = ", next);
  throw XKarma(event);
}

bool KarmaPolicyBasic::CouldPerform(const KarmaAccount &identifier, const std::string &action)
{
  accounts.access(*identifier).BringUpToDate();
  KARMA prev = accounts.access(*identifier).karma;
  KARMA next = afterwards(prev, getPolicy(action));
  return (next >= 0);
}
