#include "oef-core/karma/IKarmaPolicy.hpp"

#include <memory>

#include "oef-base/threading/Task.hpp"
#include "oef-core/karma/KarmaAccount.hpp"

IKarmaPolicy::IKarmaPolicy()
{}

IKarmaPolicy::~IKarmaPolicy()
{}

// because friendship is not heritable.
void IKarmaPolicy::changeAccountNumber(KarmaAccount *acc, std::size_t number)
{
  acc->id = number;
}

// because friendship is not heritable.
KarmaAccount IKarmaPolicy::mkAccount(std::size_t number, const std::string &name)
{
  return KarmaAccount(number, this, name);
}
