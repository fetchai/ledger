#include "semanticsearch/scope_mananger.hpp"

namespace fetch {
namespace semanticsearch {

static ScopeManagerPtr ScopeManager::New(ScopeManagerPtr const &parent = nullptr)
{
  return std::make_shared<ScopeManager>(parent);
}

ScopeManager(ScopeManagerPtr const &parent)
  : parent_{parent}
{}

bool ScopeManager::HasUniqueID(std::string const uid) const
{}

UniqueIdentifierPtr ScopeManager::GetUniqueUID(std::string const uid) const
{}

void ScopeManager::RegisterUniqueID(UniqueIdentifierPtr ptr)
{}

ScopeManagerPtr ScopeManager::NewScope()
{}

}  // namespace semanticsearch
}  // namespace fetch