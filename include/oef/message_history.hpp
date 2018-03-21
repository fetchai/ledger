// This file holds and manages connections to other nodes

#ifndef NODE_MESSAGE_HISTORY_HPP
#define NODE_MESSAGE_HISTORY_HPP

#include"oef/schema.hpp"
#include<list>
#include<algorithm>

namespace fetch
{
namespace oef
{

// TODO: (`HUT`) : delete this
void find_and_replace(std::string& source, std::string const& find, std::string const& replace)
{
  for(std::string::size_type i = 0; (i = source.find(find, i)) != std::string::npos;)
  {
    source.replace(i, find.length(), replace);
    i += replace.length();
  }
}

class Event {
public:
  Event() {}

  explicit Event(const std::string &source, const std::string &destination, const std::string &details, const std::string &id) :
    source_{source},
    destination_{destination},
    details_{details},
    id_{id} {

      const std::string find{"\""};
      const std::string repl{"'"};

      // TODO: (`HUT`) : delete
      find_and_replace(source_,      find, repl);
      find_and_replace(destination_, find, repl);
      find_and_replace(details_,     find, repl);
    }

  const std::string &source() const      { return source_; }
  std::string       &source()            { return source_; }
  const std::string &destination() const { return destination_; }
  std::string       &destination()       { return destination_; }
  const std::string &details() const     { return details_; }
  std::string       &details()           { return details_; }
  const std::string &id() const          { return id_; }
  std::string       &id()                { return id_; }

  fetch::script::Variant variant() const {
    fetch::script::Variant result = fetch::script::Variant::Object();
    result["source"]              = source_;
    result["destination"]         = destination_;
    result["details"]             = details_;
    result["id"]                  = id_;
    return result;
  }

private:
  std::string source_;
  std::string destination_;
  std::string details_;
  std::string id_;
};

class Events {

public:

  void Insert(const Event &event) {

    // Enforce size limit // TODO: (`HUT`) : template this
    if(300 == events_.size()) {
      events_.pop_front();
    }

    events_.push_back(event);
  }

  // TODO: (`HUT`) : create vector-to-variant free functions
  fetch::script::Variant variant(int maxNumber) const {

    int numberToReturn = maxNumber > events_.size() ? events_.size() : maxNumber;
    fetch::script::Variant res = fetch::script::Variant::Array(numberToReturn);

    int index = 0;
    for(auto &i : events_) {
      res[index++] = i.variant();
      if(index == numberToReturn) {
        break;
      }
    }
    return res;
  }

private:
  std::list<Event> events_;
};

template <typename T>
class MessageHistory {
public:
  explicit MessageHistory() = default;

  bool add(const T &queryModel) {

    // Check whether we have seen this before
    for(auto &i : history_) {
      if(i == queryModel) {
        return false;
      }
    }

    // Enforce size limit on history
    if(100 == history_.size()) {
      history_.pop_front();
    }

    history_.push_back(queryModel);

    return true;
  }

private:
  std::list<T> history_;
};

template< typename T>
void Serialize( T & serializer, Event const &b) {
  serializer << b.source() << b.destination() << b.details() << b.id();
}

template< typename T>
void Deserialize( T & serializer, Event &b) {
  serializer >> b.source() >> b.destination() >> b.details() >> b.id();
}

}
}
#endif

