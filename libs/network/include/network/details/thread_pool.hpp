#ifndef __NETWORK_DETAILS_THREAD_POOL__
#define __NETWORK_DETAILS_THREAD_POOL__

#include<iostream>
#include<string>
#include<functional>
#include<queue>
#include<memory>
#include<condition_variable>

#include"core/assert.hpp"
#include"core/logger.hpp"
#include"core/mutex.hpp"

#include"network/details/future_work_store.hpp"

using std::cout;
using std::cerr;
using std::string;

namespace fetch {
namespace network {
namespace details {

class ThreadPoolImplementation : public std::enable_shared_from_this< ThreadPoolImplementation >  {
 public:
  typedef std::function<void()>                        event_function_type; // TODO: (`HUT`) : curate
  typedef uint64_t                                     event_handle_type;
  typedef std::shared_ptr<ThreadPoolImplementation> shared_ptr_type;

  size_t running;

  static std::shared_ptr<ThreadPoolImplementation> Create(std::size_t threads = 1)
  {
    return std::make_shared<ThreadPoolImplementation>(threads);
  }

  ThreadPoolImplementation(std::size_t threads = 1)
      : number_of_threads_(threads) {

    fetch::logger.Debug("Creating thread manager");
    shutdown = false;
  }

  ~ThreadPoolImplementation() {
    Stop();
    fetch::logger.Debug("Destroying thread manager");
  }

  ThreadPoolImplementation(ThreadPoolImplementation const& ) = delete;
  ThreadPoolImplementation(ThreadPoolImplementation && ) = default ;

  typedef std::function<void (void)> WORK_ITEM;
  typedef std::queue<WORK_ITEM> WORK_QUEUE;
  WORK_QUEUE queue_;

  typedef std::mutex MUTEX_T;
  typedef std::unique_lock<MUTEX_T> LOCK_T;
  typedef size_t ThreadNum;
  
  mutable std::condition_variable cv;
  mutable MUTEX_T mutex_;
  volatile bool shutdown;

  typedef enum {
    THREAD_SHOULD_QUIT,
    THREAD_SHOULD_WAIT,
    THREAD_WORKED
  } ThreadState;

  void Post(WORK_ITEM item)
  {
    LOCK_T lock(mutex_);
    queue_.push(item);
    cv.notify_one();
  }

  void Sleep()
  {
    LOCK_T lock(mutex_);
    
  }

  void ProcessLoop(ThreadNum threadNumber)
  {
    {
      LOCK_T lock(mutex_);
      running++;
    }

    while(!shutdown)
      {
	switch(Poll(threadNumber))
	  {
	  case THREAD_SHOULD_QUIT:
	    {
	      return;
	    }
	  case THREAD_SHOULD_WAIT:
	    {	
	      LOCK_T lock(mutex_);
	      cv.wait_for(lock, std::chrono::milliseconds(100)); // so the future work will get serviced.
	      break; // go round again.
	    }
	  case THREAD_WORKED:
	    {
	      // no delay, go do more.
	      break;
	    }
	  }
      }
    
    {
      LOCK_T lock(mutex_);
      running--;
    }
  }

  ThreadState Poll(size_t threadNumber)
  {
    {
      LOCK_T lock(mutex_);
      if (shutdown)
	{
	  return THREAD_SHOULD_QUIT;
	}
    }

    ThreadState r = THREAD_SHOULD_WAIT;
    {
      LOCK_T lock(mutex_);
      if (!queue_.empty())
	{
	  auto workload = queue_.front();
	  queue_.pop();
	  lock.unlock();
	  try {
	    workload();
	  } catch (std::exception &x) {
	    cerr << "Caught exception inside ThreadPool::Poll" << endl;
	  }
	}
    }

    if (shutdown)
      {
	return THREAD_SHOULD_QUIT;
      }
    
    if (TryTheFutureWork(threadNumber))
      {
	r = THREAD_WORKED;
      }
    
    if (shutdown)
      {
	return THREAD_SHOULD_QUIT;
      }

    return r;
  }

  void Start()
  {
    std::lock_guard< fetch::mutex::Mutex > lock( thread_mutex_ );
    if (threads_.size() == 0) {
      fetch::logger.Info("Starting thread manager");
      shared_ptr_type self = shared_from_this();
      for (std::size_t i = 0; i < number_of_threads_; ++i) {
        threads_.push_back(new std::thread([self, i]() {
	      self -> ProcessLoop(i);
            }));
      }
    }
  }

  FutureWorkStore futureWork;
  mutable fetch::mutex::Mutex futureWorkProtector;
  
  bool TryTheFutureWork(std::size_t id)
  {
    bool r = false;
    std::unique_lock<std::mutex> lock(futureWorkProtector, std::try_to_lock);
    if (lock)
      {
	while(futureWork.isDue(id))
	  {
	    auto moreWork = futureWork.getNext(id);
	    Post(moreWork);
	    r = true;
	  }
      } // if we didn't get the lock, one thread is already doing future work -- leave it be.
    return r;
  }

  void Stop()
  {
    bool suicidal = false;
    for (auto &thread : threads_)
      {
	if (std::this_thread::get_id() == thread -> get_id())
	  {
	    suicidal = true;
	  }
      }

    if (suicidal)
      {
	fetch::logger.Warn("Thread pools must not be killed by a thread they own.");
      }
    
    std::lock_guard< fetch::mutex::Mutex > lock( thread_mutex_ );
    
    if (threads_.size() != 0)
      {
	
	fetch::logger.Info("Stopping thread pool");

	{
	  LOCK_T lock(mutex_);
	  while(!queue_.empty())
	    {
	      queue_.pop();
	    }
	  shutdown = true;
	}

	{
	  LOCK_T lock(mutex_);
	  cv.notify_all();
	}
      
	for (auto &thread : threads_)
	  {
	    if (std::this_thread::get_id() != thread -> get_id())
	      {
		thread->join();
	      }
	    else
	      {
		fetch::logger.Warn("Thread pools must not be killed by a thread they own.");
	      }
	    delete thread;
	  }

	threads_.clear();
      }
  }

  template <typename F>
  void Post(F &&f, int milliseconds)
  {
    futureWork.Post(f, milliseconds);

    LOCK_T lock(mutex_);
    cv.notify_one();    
  }

private:
  std::size_t number_of_threads_ = 1;
  std::vector<std::thread *> threads_;

  mutable fetch::mutex::Mutex thread_mutex_{__LINE__, __FILE__};
  mutable fetch::mutex::Mutex owning_mutex_{__LINE__, __FILE__};
};

}

  typedef details::ThreadPoolImplementation ThreadPool;
}
}

#endif
