#pragma once

#include <string>
#include <vector>

namespace fetch {
namespace ledger {

/**
 * A string identifier which is seperated with '.' used as to hierarchically
 * group related objects.
 *
 * For example:
 *
 *   `foo.bar` & `foo.baz` are in the same logic `foo` group
 */
class Identifier
{
public:
  using tokens_type = std::vector<std::string>;

  static constexpr char SEPERATOR = '.';

  // Construction / Destruction
  Identifier() = default;
  explicit Identifier(std::string identifier);
  Identifier(Identifier const &) = default;
  Identifier(Identifier &&)      = default;
  ~Identifier()                  = default;

  // Accessors
  std::string        name() const;
  std::string        name_space() const;
  std::string const &full_name() const;

  // Parsing
  void Parse(std::string const &name);
  void Parse(std::string &&name);

  // Comparison
  bool IsParentTo(Identifier const &other) const;
  bool IsChildTo(Identifier const &other) const;
  bool IsDirectParentTo(Identifier const &other) const;
  bool IsDirectChildTo(Identifier const &other) const;

  void Append(std::string const &element);

  // Operators
  Identifier &       operator=(Identifier const &) = default;
  Identifier &       operator=(Identifier &&) = default;
  std::string const &operator[](std::size_t index) const;
  bool               operator==(Identifier const &other) const;
  bool               operator!=(Identifier const &other) const;

private:
  std::string full_{};    ///< The fully qualified name
  tokens_type tokens_{};  ///< The individual elements of the name

  void Tokenise();
};

/**
 * Gets the top level name i.e. in the case of `foo.bar` `bar` would be
 * returned
 *
 * @return the top level name or an empty string if the identifier is empty
 */
inline std::string Identifier::name() const
{
  if (tokens_.empty())
  {
    return {};
  }
  else
  {
    return tokens_.back();
  }
}

/**
 * Gets the namespace for the identifier i.e. in the case of `foo.bar.baz`
 * `foo.bar` would be returned
 *
 * @return The namespace for the identifier
 */
inline std::string Identifier::name_space() const
{
  if (tokens_.size() >= 2)
  {
    return full_.substr(0, full_.size() - (tokens_.back().size() + 1));
  }
  else
  {
    return {};
  }
}

/**
 * Gets the fully qualified resource name
 *
 * @return The fully qualified name
 */
inline std::string const &Identifier::full_name() const { return full_; }

/**
 * Parses an fully qualified name
 *
 * @param name The fully qualified name
 */
inline void Identifier::Parse(std::string const &name)
{
  full_ = name;
  Tokenise();
}

/**
 * Parse a fully qualified name
 *
 * @param name The fully qualified name
 */
inline void Identifier::Parse(std::string &&name)
{
  full_ = std::move(name);
  Tokenise();
}

/**
 * Access elements of the name.
 *
 * @param index The index to be accessed
 * @return The element of the name
 */
inline std::string const &Identifier::operator[](std::size_t index) const
{
#ifndef NDEBUG
  return tokens_[index];
#else   // !NDEBUG
  return tokens_.at(index);
#endif  // NDEBUG
}

/**
 * Equality operator
 *
 * @param other The reference to the other identifier
 * @return true if both identifiers are the same, otherwise false
 */
inline bool Identifier::operator==(Identifier const &other) const
{
  return (full_ == other.full_);
}

/**
 * Inequality operator
 *
 * @param other The reference to the other identifier
 * @return true if identifiers are not the same, otherwise false
 */
inline bool Identifier::operator!=(Identifier const &other) const
{
  return (full_ != other.full_);
}

/**
 * Append an element to a name
 *
 * @param element The element to be added to the name
 */
inline void Identifier::Append(std::string const &element)
{
  full_.push_back(SEPERATOR);
  full_.append(element);
  tokens_.push_back(element);
}

}  // namespace ledger
}  // namespace fetch
