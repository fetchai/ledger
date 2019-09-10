#include "KarmaPolicyBasic.hpp"
#include "fetch_teams/ledger/logger.hpp"

#include "XKarma.hpp"
#include "XDisconnect.hpp"

std::atomic<std::size_t> tick_amounts(0);
std::atomic<std::size_t> tick_counter(0);

static const std::size_t MAX_KARMA = 10000;

KarmaPolicyBasic::Account::Account()
  : karma(MAX_KARMA)
  , when(0)
{
}

void KarmaPolicyBasic::Account::bringUpToDate()
{
  while(true)
  {
    KARMA old_karma = karma;
    TICKS tc = tick_counter;
    TICKS diff = tc - when;
    KARMA new_karma = std::min(karma + diff * tick_amounts, MAX_KARMA);

    // Write this as long as nothing has updated karma while we were thinking.
    if (karma.compare_exchange_strong( old_karma, new_karma, std::memory_order_seq_cst))
    {
      FETCH_LOG_INFO(LOGGING_NAME, "KARMA: Bring up to date: ticks=", diff, " step=", tick_amounts, " when=", when, " now=", tc, " old=", old_karma, " new=", new_karma);
      when = tc;
      break;
    }
  }
}

std::string KarmaPolicyBasic::getBalance(const KarmaAccount &identifier)
{
  return std::string("account=") + identifier.getName() + "  karma=" +std::to_string(accounts.access(*identifier).karma) + "/" + std::to_string(MAX_KARMA);
}

KarmaPolicyBasic::KarmaPolicyBasic(const google::protobuf::Map<std::string, std::string> &config)
{
    accounts.get("(null karma account)"); // make sure the null account exists.

    for(auto &kv : config)
    {
      this -> config[kv.first] = kv.second;
    }
}

KarmaPolicyBasic::~KarmaPolicyBasic()
{
}

void KarmaPolicyBasic::refreshCycle(const std::chrono::milliseconds delta)
{
  auto policies = getPolicies("refresh");
  tick_amounts = parseEffect(0, policies[0]);
  //FETCH_LOG_INFO(LOGGING_NAME, "KARMA: refreshTick of size ", tick_amounts);
  tick_counter++;
}

std::size_t KarmaPolicyBasic::getAccountNumber(const std::string &s)
{
  return accounts.get(s);
}

KarmaAccount KarmaPolicyBasic::getAccount(const std::string &pubkey, const std::string &ip)
{
  if (pubkey.length() > 0)
  {
    return mkAccount(getAccountNumber(pubkey), pubkey);
  }
  else if (ip.length() > 0)
  {
    return mkAccount(getAccountNumber(ip), ip);
  }
  else
  {
    return mkAccount(0, "DEFAULT_KARMA_ACCOUNT");
  }
}

void KarmaPolicyBasic::upgrade(KarmaAccount &account, const std::string &pubkey, const std::string &ip)
{
  auto k = getAccount(pubkey, ip);
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
  std::size_t end = 0;

  while(true)
  {
    start = input.find_first_not_of(delim, end);
    if (start == std::string::npos)
    {
      return;
    }
    end = input.find(delim, start); // could be npos.
    output.push_back(input.substr(start, end-start)); // npos - start is still a huge number, this gets the last token.
  }
}

std::vector<std::string> KarmaPolicyBasic::getPolicies(const std::string &action) const
{
  std::vector<std::string> actions;
  tokenise(action, actions, ',');
  if (actions.empty())
  {
    actions.push_back("*");
  }

  std::vector<std::string> policies;
  for(const auto &action : actions)
  {
    policies.push_back(getPolicy(action));
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
  return action.substr(0, r-1);
}

const std::string &KarmaPolicyBasic::getPolicy(const std::string &action) const
{
  Config::const_iterator iter;
  if ((iter = config.find(action)) != config.end())
  {
    //FETCH_LOG_INFO(LOGGING_NAME, "KARMA: Karma Event:", action, " => ", iter -> second);
    return iter -> second;
  }
  if ((iter = config.find("*")) != config.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "KARMA:  Unknown Karma Event:", action);
    return iter -> second;
  }
  FETCH_LOG_INFO(LOGGING_NAME, "KARMA:  No default karma event:", action);
  return zero;
}


KarmaPolicyBasic::KARMA KarmaPolicyBasic::afterwards(KARMA currentBalance, const std::string &actions)
{
  auto policies = getPolicies(actions);
  auto worst = currentBalance;

  try
  {
    for(const auto &policy : policies)
    {
      auto result = parseEffect(currentBalance, policy);
      FETCH_LOG_INFO(LOGGING_NAME, "KARMA: afterwards ", policy, "    ", currentBalance, " => ", result);
      if (result < worst)
      {
        worst = result;
      }
    }
  }
  catch(XKarma &x)
  {
    throw XKarma(std::string("actions:") + actions + " result in disconnect due to Karma policy");
  }

  return worst;
}


KarmaPolicyBasic::KARMA KarmaPolicyBasic::parseEffect(KARMA currentBalance, const std::string &effect)
{
  switch(effect[0])
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
  accounts.access(*identifier).bringUpToDate();
  KARMA prev = accounts.access(*identifier).karma;
  KARMA next = afterwards(prev, event);


  if (next >= 0 || force)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "KARMA: Event ", event, " for ", identifier.getName(), " karma change ", prev, " => ",  next);
    accounts.access(*identifier).karma = next;
    return true;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "KARMA: Event ", event, " for ", identifier.getName(), " rejected because result karma = ",  next);
  throw XKarma(event);
}

bool KarmaPolicyBasic::couldPerform(const KarmaAccount &identifier, const std::string &action)
{
  accounts.access(*identifier).bringUpToDate();
  KARMA prev = accounts.access(*identifier).karma;
  KARMA next = afterwards(prev, getPolicy(action));
  return (next >= 0);
}
