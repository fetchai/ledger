#ifndef EVENT_GENERATOR_HPP
#define EVENT_GENERATOR_HPP


namespace fetch
{
namespace network_test
{

class EventGenerator
{
public:
  EventGenerator(uint32_t eventsPerSecond) {}

  EventGenerator(EventGenerator &rhs)            = delete;
  EventGenerator(EventGenerator &&rhs)           = delete;
  EventGenerator operator=(EventGenerator& rhs)  = delete;
  EventGenerator operator=(EventGenerator&& rhs) = delete;

};

}
}
#endif
