#pragma once

#include "oef-base/threading/Task.hpp"

template <class ENDPOINT, class DATA_TYPE>
class TSendProtoTask : public Task
{
public:
  TSendProtoTask(DATA_TYPE pb, std::shared_ptr<ENDPOINT> endpoint)
    : Task()
    , endpoint{std::move(endpoint)}
    , pb{std::move(pb)}
  {}

  virtual ~TSendProtoTask()                   = default;
  TSendProtoTask(const TSendProtoTask &other) = delete;
  TSendProtoTask &operator=(const TSendProtoTask &other) = delete;

  bool operator==(const TSendProtoTask &other) = delete;
  bool operator<(const TSendProtoTask &other)  = delete;

  virtual bool isRunnable(void) const
  {
    return true;
  }

  virtual ExitState run(void)
  {
    // TODO(kll): it's possible there's a race hazard here. Need to think about this.
    if (endpoint->send(pb).Then([this]() { this->makeRunnable(); }).Waiting())
    {
      return ExitState::DEFER;
    }

    endpoint->run_sending();
    pb = DATA_TYPE{};
    return ExitState::COMPLETE;
  }

private:
  std::shared_ptr<ENDPOINT> endpoint;
  DATA_TYPE                 pb;
};

// namespace std { template<> void swap(TSendProto& lhs, TSendProto& rhs) { lhs.swap(rhs); } }
// std::ostream& operator<<(std::ostream& os, const TSendProto &output) {}
