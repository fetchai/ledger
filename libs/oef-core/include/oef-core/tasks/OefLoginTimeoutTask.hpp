#pragma once

#include <chrono>

#include "logging/logging.hpp"
#include "oef-base/threading/Task.hpp"

class OefAgentEndpoint;

class OefLoginTimeoutTask : public Task
{
public:
  static constexpr char const *LOGGING_NAME = "OefLoginTimeoutTask";

  OefLoginTimeoutTask(std::shared_ptr<OefAgentEndpoint> ep)
  {
    this->ep = ep;
  }
  virtual ~OefLoginTimeoutTask()
  {}

  virtual bool IsRunnable(void) const
  {
    return true;
  }
  virtual ExitState run(void);

protected:
  std::weak_ptr<OefAgentEndpoint> ep;

private:
  OefLoginTimeoutTask(const OefLoginTimeoutTask &other) = delete;
  OefLoginTimeoutTask &operator=(const OefLoginTimeoutTask &other)  = delete;
  bool                 operator==(const OefLoginTimeoutTask &other) = delete;
  bool                 operator<(const OefLoginTimeoutTask &other)  = delete;
};
