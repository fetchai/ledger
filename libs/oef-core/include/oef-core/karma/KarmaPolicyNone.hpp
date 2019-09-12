#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "logging/logging.hpp"
#include "oef-core/karma/IKarmaPolicy.hpp"
#include "oef-core/karma/KarmaAccount.hpp"

class KarmaPolicyNone : public IKarmaPolicy
{
public:
  static constexpr char const *LOGGING_NAME = "KarmaPolicyNone";
  KarmaPolicyNone();
  virtual ~KarmaPolicyNone();

  virtual KarmaAccount getAccount(const std::string &pubkey = "", const std::string &ip = "");

  virtual bool        perform(const KarmaAccount &identifier, const std::string &action,
                              bool force = false);
  virtual bool        couldPerform(const KarmaAccount &identifier, const std::string &action);
  virtual std::string getBalance(const KarmaAccount & /*identifier*/)
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
