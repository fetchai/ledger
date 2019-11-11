#pragma once

#include <memory>
#include <vector>

namespace fetch {
namespace semanticsearch {

class ScopeManager;

class UniqueIdentifier
{
public:
  using Taxonomy            = std::vector<std::string>;
  using UniqueIdentifierPtr = std::shared_ptr<UniqueIdentifier>;
  using TypeId              = uint64_t;
  using ScopeManagerPtßr    = std::shared_ptr<ScopeManager>;

  enum Kind
  {
    SCHEMA,
    INSTANCE,
    REDUCER,
    NAMESPACE
  };

  UniqueIdentifierPtr Parse(std::string str_uid, ScopeManagerPtr scope_manager);

  /// Comparison
  /// @{
  bool operator==(UniqueIdentifier const &other) const;
  bool operator<=(UniqueIdentifier const &other) const;
  bool operator<(UniqueIdentifier const &other) const;
  bool operator>=(UniqueIdentifier const &other) const;
  bool operator>(UniqueIdentifier const &other) const;
  /// @}

  /// Kind
  /// @{
  Kind kind() const;
  bool IsSchema() const;
  bool IsInstance() const;
  bool IsReducer() const;
  bool IsNamespace() const;
  /// @}

  Taxonomy const &taxonomy() const;

protected:
  void SetID(TypeId uid)
  {
    uid_ = uid;
  }

private:
  UniqueIdentifier(std::string str_uid, Kind kind, Taxonomy taxonomy)
    : str_uid_{std::move(str_uid)}
    , kind_{kind}
    , taxonomy_{std::move(taxonomy)}
  {}

  TypeId      uid_;
  std::string str_uid_;
  Kind        kind_;
  Taxonomy    taxonomy_;

  friend class ScopeManager;
};

}  // namespace semanticsearch
}  // namespace fetch