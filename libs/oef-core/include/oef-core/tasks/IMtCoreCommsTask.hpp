#pragma once

#include "oef-base/comms/EndpointBase.hpp"
#include "oef-core/tasks-base/IMtCoreTask.hpp"

#include <google/protobuf/message.h>

class IMtCoreCommsTask : public IMtCoreTask
{
public:
  IMtCoreCommsTask(std::shared_ptr<EndpointBase<google::protobuf::Message>> endpoint)
    : IMtCoreTask()
  {
    this->endpoint = endpoint;
  }
  virtual ~IMtCoreCommsTask()
  {}

protected:
  std::shared_ptr<EndpointBase<google::protobuf::Message>> endpoint;

private:
  IMtCoreCommsTask(const IMtCoreCommsTask &other) = delete;
  IMtCoreCommsTask &operator=(const IMtCoreCommsTask &other)  = delete;
  bool              operator==(const IMtCoreCommsTask &other) = delete;
  bool              operator<(const IMtCoreCommsTask &other)  = delete;
};
