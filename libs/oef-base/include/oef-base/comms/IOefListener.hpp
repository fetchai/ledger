#pragma once

#include <functional>
#include <memory>


template <class IOefTaskFactory, class OefEndpoint>
class IOefListener
{
public:
  using FactoryCreator = std::function<std::shared_ptr<IOefTaskFactory> (std::shared_ptr<OefEndpoint>)>;

  IOefListener()
  {
  }
  virtual ~IOefListener()
  {
  }

  FactoryCreator factoryCreator;

protected:
private:
  IOefListener(const IOefListener &other) = delete;
  IOefListener &operator=(const IOefListener &other) = delete;
  bool operator==(const IOefListener &other) = delete;
  bool operator<(const IOefListener &other) = delete;
};
