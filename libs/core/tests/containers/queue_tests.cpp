//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/containers/queue.hpp"

#include "gtest/gtest.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

namespace {

class QueueTests : public ::testing::Test
{
public:
  static constexpr std::size_t ELEMENT_SIZE = 1024;
  using Element                             = std::array<uint16_t, ELEMENT_SIZE>;

  template <typename Element>
  void VerifyElementConsistency(Element const &element, uint16_t &acqired_thread_id)
  {
    // thread the thread index counter
    ASSERT_EQ(element.front(), element.back());

    // check the contents of the array
    acqired_thread_id = element[1];
    for (std::size_t j = 2; j < (ELEMENT_SIZE - 1); ++j)
    {
      ASSERT_EQ(acqired_thread_id, element[j]);
    }
  }

  template <std::size_t NUM_PROD_THREADS, std::size_t NUM_CONS_THREADS, typename Queue>
  void ProducerConsumerTest(Queue &queue)
  {
    constexpr std::size_t NUM_LOOPS = 500;

    // ensure the queue element type is valid
    static_assert(std::is_same<typename Queue::Element, Element>::value, "");

    using ThreadPtr  = std::unique_ptr<std::thread>;
    using ThreadList = std::vector<ThreadPtr>;

    static const std::size_t NUM_ELEMENTS_PER_THREAD =
        (Queue::QUEUE_LENGTH * NUM_LOOPS) / NUM_PROD_THREADS;

    ThreadList  threads(NUM_PROD_THREADS);
    uint16_t    thread_idx{0};
    std::size_t num_of_elements = Queue::QUEUE_LENGTH * NUM_LOOPS;
    std::size_t num_elements_per_thread{num_of_elements / threads.size()};
    for (auto &thread : threads)
    {
      if (num_of_elements < num_elements_per_thread)
      {
        num_elements_per_thread = num_of_elements;
      }
      num_of_elements -= num_elements_per_thread;

      thread = std::make_unique<std::thread>([&queue, thread_idx, num_elements_per_thread]() {
        // create the boiler plate element
        Element element;
        std::fill(element.begin(), element.end(), thread_idx);

        for (std::size_t i = 0; i < num_elements_per_thread; ++i)
        {
          // uniquely identify the element
          element.front() = static_cast<uint16_t>(i);
          element.back()  = static_cast<uint16_t>(i);

          // add the element to the queue
          queue.Push(element);
        }
      });

      ++thread_idx;
    }

    // create the counters
    std::array<std::size_t, NUM_PROD_THREADS> counters{};
    // check initialisation
    for (auto const &count : counters)
    {
      ASSERT_EQ(count, 0);
    }
    std::mutex counter_mutex;

    ThreadList threads_consumer(NUM_CONS_THREADS);
    num_of_elements         = Queue::QUEUE_LENGTH * NUM_LOOPS;
    num_elements_per_thread = num_of_elements / threads_consumer.size();
    for (auto &thread : threads_consumer)
    {
      if (num_of_elements < num_elements_per_thread)
      {
        num_elements_per_thread = num_of_elements;
      }
      num_of_elements -= num_elements_per_thread;

      thread = std::make_unique<std::thread>(
          [this, &queue, &counters, &counter_mutex, num_elements_per_thread]() {
            Element element;
            for (std::size_t i = 0; i < num_elements_per_thread; ++i)
            {
              // pop the element from the queue
              ASSERT_TRUE(queue.Pop(element, std::chrono::seconds{4}));
              uint16_t acquired_thread_id{NUM_PROD_THREADS + 1};
              VerifyElementConsistency(element, acquired_thread_id);
              // update the thread counters
              {
                std::lock_guard<std::mutex> lock(counter_mutex);
                counters.at(acquired_thread_id) += 1;
              }
            }
          });
    }
    // wait for all the threads to complete
    for (auto &thread : threads)
    {
      thread->join();
    }
    threads.clear();

    // wait for all the consumer threads to complete
    for (auto &thread : threads_consumer)
    {
      thread->join();
    }
    threads_consumer.clear();

    // ensure all the counters are correct
    for (auto const &count : counters)
    {
      ASSERT_EQ(count, NUM_ELEMENTS_PER_THREAD);
    }
  }

  static constexpr std::size_t QUEUE_SIZE = 1024;
};

TEST_F(QueueTests, ProducerConsumer_50p_50c)
{
  fetch::core::MPMCQueue<Element, QUEUE_SIZE> queue;
  ProducerConsumerTest<50, 50>(queue);
}

TEST_F(QueueTests, ProducerConsumer_50p_2c)
{
  fetch::core::MPMCQueue<Element, QUEUE_SIZE> queue;
  ProducerConsumerTest<50, 2>(queue);
}

TEST_F(QueueTests, ProducerConsumer_2p_50c)
{
  fetch::core::MPMCQueue<Element, QUEUE_SIZE> queue;
  ProducerConsumerTest<2, 50>(queue);
}

TEST_F(QueueTests, ProducerConsumer_50p_1c_MPSCQueue)
{
  fetch::core::MPSCQueue<Element, QUEUE_SIZE> queue;
  ProducerConsumerTest<50, 1>(queue);
}

TEST_F(QueueTests, ProducerConsumer_50p_1c_MPMCQueue)
{
  fetch::core::MPMCQueue<Element, QUEUE_SIZE> queue;
  ProducerConsumerTest<50, 1>(queue);
}

TEST_F(QueueTests, ProducerConsumer_1p_1c_SPSCQueue)
{
  fetch::core::SPSCQueue<Element, QUEUE_SIZE> queue;
  ProducerConsumerTest<1, 1>(queue);
}

TEST_F(QueueTests, ProducerConsumer_1p_1c_MPMCQueue)
{
  fetch::core::MPMCQueue<Element, QUEUE_SIZE> queue;
  ProducerConsumerTest<1, 1>(queue);
}

TEST_F(QueueTests, ProducerConsumer_1p_50c_SPMCQueue)
{
  fetch::core::SPMCQueue<Element, QUEUE_SIZE> queue;
  ProducerConsumerTest<1, 50>(queue);
}

TEST_F(QueueTests, ProducerConsumer_1p_50c_MPMCQueue)
{
  fetch::core::SPMCQueue<Element, QUEUE_SIZE> queue;
  ProducerConsumerTest<1, 50>(queue);
}

}  // namespace
