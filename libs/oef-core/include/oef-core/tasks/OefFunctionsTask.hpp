#pragma once

#include "oef-core/tasks-base/IMtCoreTask.hpp"

class OefFunctionsTask : public IMtCoreTask
{
public:
  OefFunctionsTask()
  {}
  virtual ~OefFunctionsTask()
  {}

protected:
private:
  OefFunctionsTask(const OefFunctionsTask &other) = delete;
  OefFunctionsTask &operator=(const OefFunctionsTask &other)  = delete;
  bool              operator==(const OefFunctionsTask &other) = delete;
  bool              operator<(const OefFunctionsTask &other)  = delete;
};
