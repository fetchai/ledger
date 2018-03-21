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

  explicit Event(const std::string &source, const std::string &destination, const std::string &details) :
    source_{source},
    destination_{destination},
    details_{details} {

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

  fetch::script::Variant variant() const {
    fetch::script::Variant result = fetch::script::Variant::Object();
    result["source"]              = source_;
    result["destination"]         = destination_;
    result["details"]             = details_;
    return result;
  }

private:
  std::string source_;
  std::string destination_;
  std::string details_;
};

class Events {

public:

  void Insert(const Event &event) {
    events_.push_back(event);
  }

  // TODO: (`HUT`) : create vector-to-variant free functions
  fetch::script::Variant variant(int maxNumber) const {

    //fetch::script::Variant res = fetch::script::Variant::Array(std::max(int(events_.size()), maxNumber));
    fetch::script::Variant res = fetch::script::Variant::Array(events_.size());

    int index = 0;
    for(auto &i : events_) {
      res[index++] = i.variant();
    }

    return res;
  }

private:
  std::vector<Event> events_;
};

// TODO: (`HUT`) : template this class
class MessageHistory {
public:
  explicit MessageHistory() = default;

  bool add(const schema::QueryModelMulti &queryModel) {

    // Check whether we have seen this before
    for(auto &i : history_) {
      if(i == queryModel) {
        return false;
      }
    }

    // Enforce size limit on history
    if(100 == history_.size()) {
      history_.pop_back();
    }

    history_.push_back(queryModel);

    return true;
  }

private:
  std::list<schema::QueryModelMulti> history_;
};

template< typename T>
void Serialize( T & serializer, Event const &b) {
  serializer << b.source() << b.destination() << b.details();
}

template< typename T>
void Deserialize( T & serializer, Event &b) {
  serializer >> b.source() >> b.destination() >> b.details();
}

}
}
#endif

