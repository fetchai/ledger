#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <iostream>

class RingBuffer
{
public:
  using Mutex = std::mutex;
  using Lock = std::lock_guard<Mutex>;
  using buffer = boost::asio::const_buffer;
  using mutable_buffer = boost::asio::mutable_buffer;

  using byte = std::uint8_t;

  using SignalReady = std::function<void ()>;

protected:
  void ignored()
  {
  }
public:
  RingBuffer(const RingBuffer &other) = delete;
  RingBuffer &operator=(const RingBuffer &other) = delete;
  bool operator==(const RingBuffer &other) = delete;
  bool operator<(const RingBuffer &other) = delete;

  RingBuffer(size_t size)
  {
    this -> size = size;
    store = (byte*)malloc(size);
    clear();
  }

  virtual ~RingBuffer()
  {
    free(store);
  }

  void clear(void) {
    this -> freeSpace = this -> size;
    this -> writep = 0;
    this -> readp = 0;
  }

  bool empty(void) const { return getFreeSpace() == size; }

  mutable_buffer getSpaceBuffer() {
    Lock lock(mut);
    if (getFreeSpace() == 0)
      return mutable_buffer(0,0);
    return mutable_buffer( addressOf(writep%size), std::min(writep+lockless_getFreeSpace(), size) - writep );
  }

  buffer getDataBuffer()
  {
    Lock lock(mut);
    if (getFreeSpace() == size)
      return buffer(0,0);
    return buffer( addressOf(readp%size), std::min(readp+lockless_getDataAvailable(), size) - readp );
  }

  std::vector<mutable_buffer> getSpaceBuffers()
  {
    Lock lock(mut);
    std::vector<mutable_buffer> r;
    if (lockless_getFreeSpace() > 0)
      {
        size_t buffer1size = std::min(writep + lockless_getFreeSpace(), size) - writep;
        size_t buffer2size = lockless_getFreeSpace() - buffer1size;
        r.push_back(mutable_buffer( addressOf(writep), buffer1size ));
        if (buffer2size) {
          r.push_back(mutable_buffer(addressOf(0), buffer2size));
        }
      }
    return r;
  }

  std::vector<buffer> getDataBuffers()
  {
    Lock lock(mut);
    std::vector<buffer> r;
    if (lockless_getDataAvailable() < size)
      {
        size_t buffer1size = std::min(readp + lockless_getDataAvailable(), size) - readp;
        size_t buffer2size = lockless_getDataAvailable() - buffer1size;
        r.push_back(buffer( addressOf(readp), buffer1size ));
        if (buffer2size) {
          r.push_back(buffer(addressOf(0), buffer2size));
        }
      }
    return r;
  }

  void markSpaceUsed(size_t amount)
  {
    std::size_t prevAvail = 0;
    {
      Lock lock(mut);
      writep += amount;
      writep %= size;
      prevAvail = lockless_getDataAvailable();
      freeSpace -= amount;
    }
    if (!prevAvail)
      {
        signalDataReady();
      }
  }

  void markDataUsed(size_t amount)
  {
    std::size_t prevSpace = 0;
    {
      Lock lock(mut);
      readp += amount;
      readp %= size;
      prevSpace = lockless_getFreeSpace();
      freeSpace += amount;
    }
    if (!prevSpace)
      {
        signalSpaceReady();
      }
  }

  const void *addressOf(size_t index) const { return (byte*)store + index; }
  void *addressOf(size_t index) { return (byte*)store + index; }

  size_t getFreeSpace() const
  {
    Lock lock(mut);
    return lockless_getFreeSpace();
  }
  size_t getDataAvailable() const
  {
    Lock lock(mut);
    return lockless_getDataAvailable();
  }

  bool hasFreeSpace() const
  {
    Lock lock(mut);
    return lockless_getFreeSpace() > 0;
  }
  bool hasDataAvailable() const
  {
    Lock lock(mut);
    return lockless_getDataAvailable() > 0;
  }

protected:
  size_t size;
  size_t freeSpace;
  size_t readp, writep;
  byte *store;
  mutable Mutex mut;

  SignalReady signalSpaceReady = [](){};
  SignalReady signalDataReady = [](){};

  size_t lockless_getFreeSpace() const
  {
    return freeSpace;
  }
  size_t lockless_getDataAvailable() const
  {
    return size-freeSpace;
  }
};
