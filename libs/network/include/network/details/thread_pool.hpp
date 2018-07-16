#ifndef NETWORK_DETAILS_THREAD_POOL__
#define NETWORK_DETAILS_THREAD_POOL__

#include<iostream>
#include<string>
#include<functional>
#include<queue>
#include<list>
#include<memory>
#include<condition_variable>

#include"core/assert.hpp"
#include"core/logger.hpp"
#include"core/mutex.hpp"

#include"network/details/future_work_store.hpp"

namespace fetch
{
namespace network
{
namespace details
{

class ThreadPoolImplementation :
        public std::enable_shared_from_this< ThreadPoolImplementation >
{
protected:
    typedef std::function<void()>                        event_function_type;
    typedef uint64_t                                     event_handle_type;
    typedef std::shared_ptr<ThreadPoolImplementation>    shared_ptr_type;
    typedef std::mutex                                   mutex_type;
    typedef std::unique_lock<mutex_type>                 lock_type;
    typedef std::queue<event_function_type>              work_queue_type;
    typedef enum
        {
            THREAD_IDLE = 0x00,
            THREAD_WORKED = 0x01,
            THREAD_SHOULD_QUIT = 0x02,
        } thread_state_type;
    typedef std::list<event_function_type>              idle_work_type;

public:
    static std::shared_ptr<ThreadPoolImplementation> Create(std::size_t threads)
    {
        return std::make_shared<ThreadPoolImplementation>(threads);
    }

    ThreadPoolImplementation(std::size_t threads) : number_of_threads_(threads)
    {

        fetch::logger.Debug("Creating thread manager");
        shutdown_ = false;
    }

    virtual ~ThreadPoolImplementation()
    {
        Stop();
        fetch::logger.Debug("Destroying thread manager");
    }

    ThreadPoolImplementation(ThreadPoolImplementation const& ) = delete;
    ThreadPoolImplementation(ThreadPoolImplementation && ) = default ;

    virtual void Post(event_function_type item)
    {
        if (!shutdown_)
        {
            lock_type lock(mutex_);
            queue_.push(item);
            cv_.notify_one();
        }
    }

    virtual void Sleep()
    {
        lock_type lock(mutex_);
    }

    virtual void SetIdleWork(event_function_type idle_work)
    {
        lock_type lock(mutex_);
        idleWork_.push_back(idle_work);
    }

    virtual void ProcessLoop()
    {
        while(!shutdown_)
        {
            thread_state_type state = Poll();
            if (state & THREAD_SHOULD_QUIT)
            {
                return; // we're done -- return & become joinable.
            }
            if (state & THREAD_WORKED)
            {
                // no delay, go do more.
                continue;
            }
            // THREAD_IDLE:
            {
                // snooze for a while or until more work arrives
                lock_type lock(mutex_);
                cv_.wait_for(lock, std::chrono::milliseconds(100));
                // so the future work will get serviced eventually.
                // go round again.
            }
        }
    }

    virtual thread_state_type Poll()
    {
        {
            lock_type lock(mutex_);
            if (shutdown_)
            {
                return THREAD_SHOULD_QUIT;
            }
        }

        thread_state_type r = THREAD_IDLE;
        {
            lock_type lock(mutex_);
            if (!queue_.empty())
            {
                auto workload = queue_.front();
                queue_.pop();
                lock.unlock();
                r = std::max(r, ExecuteWorkload(workload));
            }
        }

        if (shutdown_)
        {
            return THREAD_SHOULD_QUIT;
        }

        if (!r)
        {
            r = std::max(r, TryFutureWork());
        }

        if (shutdown_)
        {
            return THREAD_SHOULD_QUIT;
        }

        if (!r)
        {
            r = std::max(r, TryIdleWork());
        }

        if (shutdown_)
        {
            return THREAD_SHOULD_QUIT;
        }

        return r;
    }

    virtual void Start()
    {
        std::lock_guard< fetch::mutex::Mutex > lock( thread_mutex_ );
        if (threads_.size() == 0)
        {
            fetch::logger.Info("Starting thread manager");
            shared_ptr_type self = shared_from_this();
            for (std::size_t i = 0; i < number_of_threads_; ++i)
            {
                threads_.push_back(new std::thread([self](){
                            self -> ProcessLoop();
                        })
                    );
            }
        }
    }

    virtual thread_state_type TryIdleWork()
    {
        thread_state_type r = THREAD_IDLE;
        std::unique_lock<std::mutex> lock(
            futureWorkProtector_, std::try_to_lock);
        if (lock)
        {
            for(auto workload : idleWork_)
            {
                workload();
                // workload may be longer running, we
                // need to test for exits here.
                if (shutdown_)
                {
                    return THREAD_SHOULD_QUIT;
                }
                r = THREAD_WORKED;
            }
        }  // if we didn't get the lock, a thread
        // is already doing this -- leave it be.
        return r;
    }

    virtual thread_state_type TryFutureWork()
    {
        thread_state_type r = THREAD_IDLE;
        std::unique_lock<std::mutex> lock(
            futureWorkProtector_, std::try_to_lock);
        if (lock)
        {
            while(futureWork_.IsDue())
            {
                // removes work from the pool and returns it.
                auto moreWork = futureWork_.GetNext();
                Post(moreWork); // We give that work to the other threads.
                r = THREAD_WORKED; // We did something.
            }
        }
        // if we didn't get the lock, one thread is already
        //doing future work -- leave it be.
        return r;
    }

    void Stop()
    {
        bool tryingToKillFromThreadWeOwn = false;
        shutdown_ = true;
        for (auto &thread : threads_)
        {
            if (std::this_thread::get_id() == thread -> get_id())
            {
                tryingToKillFromThreadWeOwn = true;
            }
        }

        if (tryingToKillFromThreadWeOwn)
        {
            fetch::logger.Error(
                "Thread pools must not be killed by a thread they own.");
        }

        std::lock_guard< fetch::mutex::Mutex > lock( thread_mutex_ );
        if (threads_.size() != 0)
        {

            fetch::logger.Info("Stopping thread pool");

            {
                lock_type lock(mutex_);
                while(!queue_.empty())
                {
                    queue_.pop();
                }
            }

            {
                lock_type lock(mutex_);
                cv_.notify_all();
            }

            for (auto &thread : threads_)
            {
                if (std::this_thread::get_id() != thread -> get_id())
                {
                    thread->join();
                    delete thread;
                }
                else
                {
                    fetch::logger.Error(
                        "Thread pools must not be killed by a thread they own"
                        "so I'm not going to try joining myself.");
                    thread->detach();
                    delete thread;
                }
            }

            threads_.clear();
        }
    }

    template <typename F>
    void Post(F &&f, uint32_t milliseconds)
    {
        if (!shutdown_)
        {
            futureWork_.Post(f, milliseconds);
            lock_type lock(mutex_);
            cv_.notify_one();
        }
    }

private:
    virtual thread_state_type ExecuteWorkload(event_function_type workload)
    {
        if (shutdown_)
        {
            return THREAD_SHOULD_QUIT;
        }

        thread_state_type r = THREAD_IDLE;
        try
        {
            workload();
            r = std::max(r, THREAD_WORKED);
        }
        catch (...)
        {
            fetch::logger.Error(
                "Caught exception in ThreadPool::ExecuteWorkload");
        }

        if (shutdown_)
        {
            return THREAD_SHOULD_QUIT;
        }

        return r;
    }

    std::size_t number_of_threads_ = 1;
    std::vector<std::thread *> threads_;

    FutureWorkStore futureWork_;
    work_queue_type queue_;
    idle_work_type idleWork_;

    mutable fetch::mutex::Mutex futureWorkProtector_;
    mutable fetch::mutex::Mutex thread_mutex_{__LINE__, __FILE__};
    mutable std::condition_variable cv_;
    mutable mutex_type mutex_;
    std::atomic<bool> shutdown_;
};

}

    typedef details::ThreadPoolImplementation ThreadPool;
}
}

#endif
