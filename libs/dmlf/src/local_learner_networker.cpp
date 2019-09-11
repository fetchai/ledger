#include "dmlf/local_learner_networker.hpp"

#include <iostream>

#include "dmlf/iupdate.hpp"

namespace fetch {
namespace dmlf {

LocalLearnerNetworker::LocalLearnerNetworker()
{
  this -> ident = getCounter()++;
}

LocalLearnerNetworker::~LocalLearnerNetworker()
{
}

void LocalLearnerNetworker::pushUpdate( std::shared_ptr<IUpdate> update)
{
  std::cout << update << std::endl;
}

std::size_t LocalLearnerNetworker::getUpdateCount() const
{
  return 0;
}

std::shared_ptr<IUpdate> LocalLearnerNetworker::getUpdate()
{
  return std::make_shared<IUpdate>();
}

std::size_t LocalLearnerNetworker::getCount()
{
  return getCounter();
}

std::size_t &LocalLearnerNetworker::getCounter()
{
  static std::size_t counter = 0;
  return counter;
}

}
}
