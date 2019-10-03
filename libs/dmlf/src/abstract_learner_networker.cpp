#include "dmlf/abstract_learner_networker.hpp"

namespace fetch {
namespace dmlf {
  void AbstractLearnerNetworker::PushUpdateType(const std::string & /*key*/,
                                                const std::shared_ptr<UpdateInterface> & /*update*/)
  {
    // do nothing in the base case.
  }

  std::size_t AbstractLearnerNetworker::GetUpdateCount() const
  {
    FETCH_LOCK(queue_m_);
    ThrowIfNotInitialized();
    return queue_->size();
  }

  void AbstractLearnerNetworker::SetShuffleAlgorithm(
       const std::shared_ptr<ShuffleAlgorithmInterface> &alg)
  {
    alg_ = alg;
  }

  std::size_t AbstractLearnerNetworker::GetUpdateTypeCount(const std::string &key) const
  {
    FETCH_LOCK(queue_map_m_);
    auto iter = queue_map_.find(key);
    if (iter != queue_map_.end())
    {
      return iter->second->size();
    }
    throw std::runtime_error{"Requesting UpdateCount for unregistered type"};
  }

  void AbstractLearnerNetworker::NewMessage(const Bytes &msg)
  {
    FETCH_LOCK(queue_m_);
    ThrowIfNotInitialized();
    queue_->PushNewMessage(msg);
  }

  void AbstractLearnerNetworker::NewDmlfMessage(const Bytes &msg)
  {
    serializers::MsgPackSerializer serializer{msg};
    std::string                    key;
    Bytes                          update;

    serializer >> key;
    serializer >> update;

    FETCH_LOCK(queue_map_m_);
    auto iter = queue_map_.find(key);
    if (iter != queue_map_.end())
    {
      iter->second->PushNewMessage(update);
      return;
    }
    throw std::runtime_error{"Received Update with a non-registered type"};
  }

  void AbstractLearnerNetworker::ThrowIfNotInitialized() const
  {
    if (!queue_)
    {
      throw std::runtime_error{"Learner is not initialized"};
    }
  }
}  // namespace dmlf
}  // namespace fetch
