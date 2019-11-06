#include "dmlf/colearn/muddle_learner_networker_impl.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

  MuddleLearnerNetworkerImpl::MuddleLearnerNetworkerImpl(MuddlePtr mud)
    : mud_(mud)
  {
    taskpool = std::make_shared<Taskpool>();
    tasks_runners = std::make_shared<Threadpool>();
    std::function<void(std::size_t thread_number)> run_tasks =
      std::bind(&Taskpool::run, taskpool.get(), std::placeholders::_1);
    tasks_runners -> start(5, run_tasks);
  }
  MuddleLearnerNetworkerImpl::~MuddleLearnerNetworkerImpl()
  {
    taskpool -> stop();
    ::usleep(10000);
    tasks_runners -> stop();
  }

  void MuddleLearnerNetworkerImpl::submit(const TaskP&t)
  {
    taskpool -> submit(t);

  }
}
}
}

