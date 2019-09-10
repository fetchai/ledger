#pragma once

#include "mt-core/tasks-base/src/cpp/IMtCoreTask.hpp"

class OEFSearchTask: public IMtCoreTask
{
public:
  OEFSearchTask()
  {
  }
  virtual ~OEFSearchTask()
  {
  }
protected:
private:
  OEFSearchTask(const OEFSearchTask &other) = delete;
  OEFSearchTask &operator=(const OEFSearchTask &other) = delete;
  bool operator==(const OEFSearchTask &other) = delete;
  bool operator<(const OEFSearchTask &other) = delete;

};
