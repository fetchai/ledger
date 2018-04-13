#ifndef EVENT_GENERATOR_HPP
#define EVENT_GENERATOR_HPP


namespace fetch
{
namespace network_test
{

class EventGenerator final
{
public:

  EventGenerator()                               = default;
  EventGenerator(EventGenerator &rhs)            = delete;
  EventGenerator(EventGenerator &&rhs)           = delete;
  EventGenerator operator=(EventGenerator& rhs)  = delete;
  EventGenerator operator=(EventGenerator&& rhs) = delete;

  ~EventGenerator()
  {
    finished_ = true;
    cond_.notify_all();

    if(thread_)
    {
      thread_->join();
    }
  }

  void start()
  {
    running_  = true;
    finished_ = false;

    if(thread_)
    {
      thread_->join();
    }

    thread_   = std::unique_ptr<std::thread>(new std::thread([this]() { run();}));
  }

  void stop()
  {
    finished_ = true;
    //cond_.notify_all();
  }

  void event(const std::function<void(void)>& arg)
  {
    if(event_ == nullptr && !running_)
    {
      event_ = arg;
    }
  }

  void eventPeriodUs(uint64_t period)
  {
    eventPeriodUs_ = period;
  }

private:
  bool                         running_       = false;
  bool                         finished_      = false;
  uint32_t                     eventPeriodUs_ = 100;
  std::chrono::microseconds    microsecond_{1};
  std::mutex                   mutex_;
  std::condition_variable      cond_;
  std::function<void(void)>    event_         = nullptr;
  std::unique_ptr<std::thread> thread_;

  /*void run()
  {
    while(finished_ == false)
    {

      std::unique_lock<std::mutex> mlock(mutex_);

      cond_.wait_for(mlock,microsecond_*eventPeriodUs_);

      if(finished_ == false)
      {
        if(event_)
        {
          event_();
        }
      }
    }
  } */

  void run()
  {
    running_ = true;
    while(finished_ == false)
    {
      if(eventPeriodUs_ > 0)
      {
        std::this_thread::sleep_for(std::chrono::microseconds(eventPeriodUs_));
      }

      if(event_)
      {
        event_();
      }
    }

    std::cerr << "Thread finished" << std::endl;
    running_ = false;
  }

};

}
}
#endif

