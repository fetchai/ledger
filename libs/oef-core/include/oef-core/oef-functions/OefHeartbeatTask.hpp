#pragma once

#include <chrono>

#include "fetch_teams/ledger/logger.hpp"
#include "oef-base/threading/Task.hpp"

class OefAgentEndpoint;

class OefHeartbeatTask : public Task
{
public:
  static constexpr char const *LOGGING_NAME = "OefHeartbeatTask";

  OefHeartbeatTask(std::shared_ptr<OefAgentEndpoint> ep)
  {
    this->ep = ep;
  }
  virtual ~OefHeartbeatTask()
  {}

  virtual bool isRunnable(void) const
  {
    return true;
  }
  virtual ExitState run(void);

protected:
  std::weak_ptr<OefAgentEndpoint> ep;

private:
  OefHeartbeatTask(const OefHeartbeatTask &other) = delete;
  OefHeartbeatTask &operator=(const OefHeartbeatTask &other)  = delete;
  bool              operator==(const OefHeartbeatTask &other) = delete;
  bool              operator<(const OefHeartbeatTask &other)  = delete;
};
