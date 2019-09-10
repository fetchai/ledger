#pragma once

#include <chrono>

#include "base/src/cpp/threading/Task.hpp"
#include "fetch_teams/ledger/logger.hpp"

class IKarmaPolicy;

class KarmaRefreshTask
  : public Task
{
public:
  static constexpr char const *LOGGING_NAME = "KarmaRefreshTask";

  KarmaRefreshTask(IKarmaPolicy *policy, std::size_t interval)
  {
    this -> interval = interval;
    this -> policy = policy;
    FETCH_LOG_INFO(LOGGING_NAME, "KarmaRefreshTask CREATED, interval=", interval);
    last_execute = std::chrono::high_resolution_clock::now();
  }
  virtual ~KarmaRefreshTask()
  {
  }

  virtual bool isRunnable(void) const
  {
    return true;
  }
  virtual ExitState run(void);

protected:
  std::chrono::high_resolution_clock::time_point last_execute;
  IKarmaPolicy *policy;
  std::size_t interval;
private:
  KarmaRefreshTask(const KarmaRefreshTask &other) = delete;
  KarmaRefreshTask &operator=(const KarmaRefreshTask &other) = delete;
  bool operator==(const KarmaRefreshTask &other) = delete;
  bool operator<(const KarmaRefreshTask &other) = delete;
};
