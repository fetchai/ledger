#pragma once

#include "mt-core/tasks-base/src/cpp/IMtCoreTask.hpp"

class OefAgentEndpoint;

template<class PROTOBUF>
class TSendProtoTask : public IMtCoreTask
{
public:
  using Proto = PROTOBUF;
  using ProtoP = std::shared_ptr<Proto>;

  TSendProtoTask(ProtoP pb, std::shared_ptr<OefAgentEndpoint> endpoint) : IMtCoreTask()
  {
    this -> endpoint = endpoint;
    this -> pb = pb;
    this -> foo = pb.get();
  }

  virtual ~TSendProtoTask()
  {
  }

  virtual bool isRunnable(void) const
  {
    return true;
  }

  virtual ExitState run(void)
  {
    // TODO(kll): it's possible there's a race hazard here. Need to think about this.
    if (endpoint -> send(pb) . Then([this](){ this -> makeRunnable(); }). Waiting())
    {
      return ExitState::DEFER;
    }

    endpoint -> run_sending();
    pb.reset();
    return ExitState::COMPLETE;
  }

protected:
  std::shared_ptr<OefAgentEndpoint> endpoint;
  google::protobuf::Message *foo;
private:
  ProtoP pb;

  TSendProtoTask(const TSendProtoTask &other) = delete;
  TSendProtoTask &operator=(const TSendProtoTask &other) = delete;
  bool operator==(const TSendProtoTask &other) = delete;
  bool operator<(const TSendProtoTask &other) = delete;
};

//namespace std { template<> void swap(TSendProto& lhs, TSendProto& rhs) { lhs.swap(rhs); } }
//std::ostream& operator<<(std::ostream& os, const TSendProto &output) {}
