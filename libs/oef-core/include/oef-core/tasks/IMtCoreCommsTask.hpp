#pragma once

#include "oef-messages/fetch_protobuf.hpp"
#include "oef-base/comms/EndpointBase.hpp"
#include "oef-core/tasks-base/IMtCoreTask.hpp"

class IMtCoreCommsTask : public IMtCoreTask
{
public:
  IMtCoreCommsTask(std::shared_ptr<EndpointBase<google::protobuf::Message>> endpoint)
    : IMtCoreTask()
  {
    this->endpoint = endpoint;
  }
  virtual ~IMtCoreCommsTask() = default;

protected:
  std::shared_ptr<EndpointBase<google::protobuf::Message>> endpoint;

private:
  IMtCoreCommsTask(const IMtCoreCommsTask &other) = delete;
  IMtCoreCommsTask &operator=(const IMtCoreCommsTask &other)  = delete;
  bool              operator==(const IMtCoreCommsTask &other) = delete;
  bool              operator<(const IMtCoreCommsTask &other)  = delete;
};
