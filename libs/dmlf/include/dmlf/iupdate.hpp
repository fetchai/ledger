#pragma once

namespace fetch {
namespace dmlf {

class IUpdate
{
public:
  IUpdate()
  {
  }
  virtual ~IUpdate()
  {
  }
protected:
private:
  IUpdate(const IUpdate &other) = delete;
  IUpdate &operator=(const IUpdate &other) = delete;
  bool operator==(const IUpdate &other) = delete;
  bool operator<(const IUpdate &other) = delete;
};

}
}
