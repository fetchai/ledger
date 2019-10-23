#pragma once

#include "logging/logging.hpp"
#include "oef-base/threading/Task.hpp"

class MonitoringTask : public Task
{
public:
  static constexpr char const *LOGGING_NAME = "MonitoringTask";

  MonitoringTask()
  {}
  virtual ~MonitoringTask()
  {}

  virtual bool IsRunnable(void) const
  {
    return true;
  }
  virtual ExitState run(void);

protected:
private:
  MonitoringTask(const MonitoringTask &other) = delete;  // { copy(other); }
  MonitoringTask &operator                    =(const MonitoringTask &other) =
      delete;                                             // { copy(other); return *this; }
  bool operator==(const MonitoringTask &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const MonitoringTask &other)  = delete;  // const { return compare(other)==-1; }
};
