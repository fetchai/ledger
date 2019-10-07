#pragma once

#include "logging/logging.hpp"
#include "oef-core/karma/IKarmaPolicy.hpp"
#include "oef-core/karma/KarmaAccount.hpp"

class KarmaPolicyNone : public IKarmaPolicy
{
public:
  static constexpr char const *LOGGING_NAME = "KarmaPolicyNone";
  KarmaPolicyNone();
  virtual ~KarmaPolicyNone();

  virtual KarmaAccount GetAccount(const std::string &pubkey = "", const std::string &ip = "");

  virtual bool        perform(const KarmaAccount &identifier, const std::string &action,
                              bool force = false);
  virtual bool        CouldPerform(const KarmaAccount &identifier, const std::string &action);
  virtual std::string GetBalance(const KarmaAccount &identifier)
  {
    return "ACCEPT ALL";
  }

protected:
private:
  KarmaPolicyNone(const KarmaPolicyNone &other) = delete;
  KarmaPolicyNone &operator=(const KarmaPolicyNone &other)  = delete;
  bool             operator==(const KarmaPolicyNone &other) = delete;
  bool             operator<(const KarmaPolicyNone &other)  = delete;
};
