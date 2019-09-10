#pragma once

#include <utility>
#include <string>
#include <stdexcept>

class IKarmaPolicy;

class KarmaAccount
{
public:
  static constexpr char const *LOGGING_NAME = "KarmaAccount";

  KarmaAccount(const KarmaAccount &other) { copy(other); }
  KarmaAccount &operator=(const KarmaAccount &other) { copy(other); return *this; }
  bool operator==(const KarmaAccount &other) const { return compare(other)==0; }
  bool operator<(const KarmaAccount &other) const { return compare(other)==-1; }
  KarmaAccount(IKarmaPolicy *policy)
  {
    this -> id = 0;
    this -> policy = policy;
    this -> name = "NULL_KARMA_ACCOUNT";
  }

  KarmaAccount()
  {
    this -> id = 0;
    this -> policy = 0;
  }

  virtual ~KarmaAccount()
  {
  }

  std::string getBalance();

  virtual void upgrade(const std::string &pubkey="", const std::string &ip="");

  virtual bool perform(const std::string &action, bool force=false);
  virtual bool couldPerform(const std::string &action);

  friend void swap(KarmaAccount &a, KarmaAccount &b);
  std::size_t operator*() const { return id; }

  const std::string &getName(void) const { return name; }
protected:

  friend IKarmaPolicy;

  std::size_t id;
  IKarmaPolicy *policy;
  std::string name;

  int compare(const KarmaAccount &other) const
  {
    if (policy != other.policy)
    {
      throw std::logic_error("KarmaAccounts are not comparable between policies.");
    }
    if (id > other.id) return 1;
    if (id < other.id) return -1;
    return 0;
  }
  void copy(const KarmaAccount &other)
  {
    id = other.id;
    policy = other.policy;
    name = other.name;
  }
  void swap(KarmaAccount &other)
  {
    if (policy != other.policy)
    {
      throw std::logic_error("KarmaAccounts are not swappable between policies.");
    }
    std::swap(id, other.id);
    std::swap(name, other.name);
  }

  KarmaAccount(std::size_t id, IKarmaPolicy *policy, const std::string &name)
  {
    this -> id = id;
    this -> policy = policy;
    this -> name = name;
  }
};

void swap(KarmaAccount& v1, KarmaAccount& v2);
