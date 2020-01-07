#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <atomic>
#include <map>
#include <vector>

#include "oef-base/utils/BucketsOf.hpp"
#include "oef-core/karma/IKarmaPolicy.hpp"
#include "oef-core/karma/KarmaAccount.hpp"
#include "oef-messages/fetch_protobuf.hpp"

class KarmaPolicyBasic : public IKarmaPolicy
{
public:
  explicit KarmaPolicyBasic(const google::protobuf::Map<std::string, std::string> &config);
  ~KarmaPolicyBasic() override = default;

  KarmaAccount GetAccount(const std::string &pubkey = "", const std::string &ip = "") override;
  void         upgrade(KarmaAccount &account, const std::string &pubkey = "",
                       const std::string &ip = "") override;

  bool        perform(const KarmaAccount &identifier, const std::string &event,
                      bool force = false) override;
  bool        CouldPerform(const KarmaAccount &identifier, const std::string &action) override;
  std::string GetBalance(const KarmaAccount &identifier) override;

  void RefreshCycle(std::chrono::milliseconds delta) override;

protected:
private:
  using KARMA                               = int32_t;
  using TICKS                               = std::size_t;
  static constexpr char const *LOGGING_NAME = "KarmaPolicyBasic";

  class Account
  {
  public:
    mutable std::atomic<KARMA> karma;
    mutable std::atomic<TICKS> when;

    Account();

    void BringUpToDate();
  };

  using AccountName   = std::string;
  using AccountNumber = std::size_t;
  using Accounts      = BucketsOf<Account, std::string, std::size_t, 1024>;
  using Config        = std::map<std::string, std::string>;

  Accounts accounts;
  Config   config;

  AccountNumber GetAccountNumber(const AccountName &s);

  Account &      access(const KarmaAccount &identifier);
  const Account &access(const KarmaAccount &identifier) const;

  KarmaPolicyBasic(const KarmaPolicyBasic &other) = delete;
  KarmaPolicyBasic &operator=(const KarmaPolicyBasic &other)  = delete;
  bool              operator==(const KarmaPolicyBasic &other) = delete;
  bool              operator<(const KarmaPolicyBasic &other)  = delete;

  std::vector<std::string> getPolicies(const std::string &action) const;
  const std::string &      getPolicy(const std::string &action) const;
  KARMA                    parseEffect(KARMA currentBalance, const std::string &effect);
  KARMA                    afterwards(KARMA currentBalance, const std::string &actions);
  std::string              getLessSpecificAction(const std::string &action) const;
};
