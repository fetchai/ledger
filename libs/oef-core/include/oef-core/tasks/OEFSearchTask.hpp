#pragma once

#include "oef-core/tasks-base/IMtCoreTask.hpp"

class OEFSearchTask : public IMtCoreTask
{
public:
  OEFSearchTask()          = default;
  virtual ~OEFSearchTask() = default;

protected:
private:
  OEFSearchTask(const OEFSearchTask &other) = delete;
  OEFSearchTask &operator=(const OEFSearchTask &other)  = delete;
  bool           operator==(const OEFSearchTask &other) = delete;
  bool           operator<(const OEFSearchTask &other)  = delete;
};
