#pragma once

#include <boost/asio.hpp>
#include <vector>

class IMessageReader
{
public:
  using buffer = boost::asio::const_buffer;
  using buffers = std::vector<buffer>;
  using consumed_needed_pair = std::pair<std::size_t, std::size_t>;

  IMessageReader()
  {
  }
  virtual ~IMessageReader()
  {
  }

  virtual consumed_needed_pair initial() {
    return consumed_needed_pair(0, 4);
  }

  virtual consumed_needed_pair checkForMessage(const buffers &data) = 0;
protected:
private:
  IMessageReader(const IMessageReader &other) = delete;
  IMessageReader &operator=(const IMessageReader &other) = delete;
  bool operator==(const IMessageReader &other) = delete;
  bool operator<(const IMessageReader &other) = delete;
};
