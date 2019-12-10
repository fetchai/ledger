#include "semanticsearch/scope_manager.hpp"

namespace fetch {
namespace semanticsearch {

ScopeManagerPtr ScopeManager::New(ScopeManagerPtr const &parent)
{
  return std::make_shared<ScopeManager>(parent);
}

ScopeManager::ScopeManager(ScopeManagerPtr const &parent)
  : parent_{parent}
{}

bool ScopeManager::HasUniqueID(std::string const /*uid*/) const
{
  return false;
}

UniqueIdentifierPtr ScopeManager::GetUniqueID(std::string const /*uid*/) const
{
  return {};
}

void ScopeManager::RegisterUniqueID(UniqueIdentifierPtr /*ptr*/)
{}

ScopeManagerPtr ScopeManager::NewScope()
{
  return {};
}

}  // namespace semanticsearch
}  // namespace fetch