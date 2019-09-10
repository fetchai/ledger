#pragma once

#include "mt-core/karma/src/cpp/IKarmaPolicy.hpp"
#include "mt-core/karma/src/cpp/KarmaAccount.hpp"
#include "fetch_teams/ledger/logger.hpp"

class KarmaPolicyNone
  : public IKarmaPolicy
{
public:
  static constexpr char const *LOGGING_NAME = "KarmaPolicyNone";
  KarmaPolicyNone();
  virtual ~KarmaPolicyNone();

  virtual KarmaAccount getAccount(const std::string &pubkey="", const std::string &ip="");

  virtual bool perform(const KarmaAccount &identifier, const std::string &action, bool force=false);
  virtual bool couldPerform(const KarmaAccount &identifier, const std::string &action);
  virtual std::string getBalance(const KarmaAccount &identifier) { return "ACCEPT ALL"; }
protected:
private:
  KarmaPolicyNone(const KarmaPolicyNone &other) = delete;
  KarmaPolicyNone &operator=(const KarmaPolicyNone &other) = delete;
  bool operator==(const KarmaPolicyNone &other) = delete;
  bool operator<(const KarmaPolicyNone &other) = delete;
};
