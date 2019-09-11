#pragma once

namespace fetch {
namespace dmlf {

class Update
{
public:
  Update()
  {
  }
  virtual ~Update()
  {
  }
protected:
private:
  Update(const Update &other) = delete;
  Update &operator=(const Update &other) = delete;
  bool operator==(const Update &other) = delete;
  bool operator<(const Update &other) = delete;
};

}
}
