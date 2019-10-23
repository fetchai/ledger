#pragma once

#include "oef-base/threading/Task.hpp"

// All tasks in the MT core should inherit from this common one.

class IMtCoreTask : public Task
{
public:
  IMtCoreTask()
  {}
  virtual ~IMtCoreTask()
  {}

protected:
private:
  IMtCoreTask(const IMtCoreTask &other) = delete;
  IMtCoreTask &operator=(const IMtCoreTask &other)  = delete;
  bool         operator==(const IMtCoreTask &other) = delete;
  bool         operator<(const IMtCoreTask &other)  = delete;
};
