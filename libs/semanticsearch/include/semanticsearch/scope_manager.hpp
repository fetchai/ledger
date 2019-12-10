#pragma once
#include "semanticsearch/unique_identifier.hpp"

#include <memory>
#include <unordered_map>

namespace fetch {
namespace semanticsearch {

class ScopeManager;
using ScopeManagerPtr = std::shared_ptr<ScopeManager>;

class ScopeManager : std::enable_shared_from_this<ScopeManager>
{
public:
  using UniqueIdentifierPtr = std::shared_ptr<UniqueIdentifier>;
  using TypeId              = UniqueIdentifier::TypeId;
  using TypeIdToUIDMap      = std::unordered_map<TypeId, UniqueIdentifierPtr>;

  /// @{
  static ScopeManagerPtr New(ScopeManagerPtr const &parent = nullptr);
  ScopeManager() = delete;
  /// @}

  /// Id management
  /// @{
  bool                HasUniqueID(std::string const uid) const;
  UniqueIdentifierPtr GetUniqueID(std::string const uid) const;
  void                RegisterUniqueID(UniqueIdentifierPtr ptr);
  /// @}

  /// Execution scope
  /// @{
  ScopeManagerPtr NewScope();
  /// @}
private:
  ScopeManager(ScopeManagerPtr const &parent);

  /// ID registration
  /// @{
  ScopeManagerPtr parent_{nullptr};
  TypeIdToUIDMap  uids_{};
  /// @}
};

}  // namespace semanticsearch
}  // namespace fetch