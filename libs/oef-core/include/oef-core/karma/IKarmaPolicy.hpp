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

#include <chrono>
#include <string>

class KarmaAccount;

class IKarmaPolicy
{
public:
  static constexpr char const *LOGGING_NAME = "IKarmaPolicy";

  IKarmaPolicy()          = default;
  virtual ~IKarmaPolicy() = default;

  virtual KarmaAccount GetAccount(const std::string &pubkey = "", const std::string &ip = "") = 0;
  virtual void         upgrade(KarmaAccount & /*account*/, const std::string & /*pubkey*/ = "",
                               const std::string & /*ip*/ = "")
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
