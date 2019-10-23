#include "oef-core/karma/KarmaAccount.hpp"

#include "logging/logging.hpp"
#include "oef-core/karma/IKarmaPolicy.hpp"

void swap(KarmaAccount &v1, KarmaAccount &v2)
{
  v1.swap(v2);
}

bool KarmaAccount::perform(const std::string &action, bool /*force*/)
{
  if (!policy)
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

bool KarmaAccount::CouldPerform(const std::string &action)
{
  if (!policy)
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
