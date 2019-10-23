#pragma once

#include <chrono>
#include <string>

class KarmaAccount;

class IKarmaPolicy
{
public:
  static constexpr char const *LOGGING_NAME = "IKarmaPolicy";

  IKarmaPolicy();
  virtual ~IKarmaPolicy();

  virtual KarmaAccount GetAccount(const std::string &pubkey = "", const std::string &ip = "") = 0;
  virtual void         upgrade(KarmaAccount &/*account*/, const std::string &/*pubkey*/ = "",
                               const std::string &/*ip*/ = "")
  {}

  virtual std::string GetBalance(const KarmaAccount &identifier) = 0;

  // Returns True or throws
  virtual bool perform(const KarmaAccount &identifier, const std::string &action,
                       bool force = false) = 0;

  virtual bool CouldPerform(const KarmaAccount &identifier, const std::string &action) = 0;

  virtual void RefreshCycle(const std::chrono::milliseconds /*delta*/)
  {}

protected:
  // because friendship is not heritable.
  void changeAccountNumber(KarmaAccount *acc, std::size_t number);

  // because friendship is not heritable.
  KarmaAccount mkAccount(std::size_t number, const std::string &name);

private:
  IKarmaPolicy(const IKarmaPolicy &other) = delete;
  IKarmaPolicy &operator=(const IKarmaPolicy &other)  = delete;
  bool          operator==(const IKarmaPolicy &other) = delete;
  bool          operator<(const IKarmaPolicy &other)  = delete;
};
