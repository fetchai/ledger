// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
// Defines the messages that can be sent between any two entities in our system
// Note that the messages themselves shouldn't reveal anything about
// the format (JSON, binary), although this isn't neccesarily currently true
//
// A message class should provide:
// - A constructor that takes an Archive
// - the serialize template that will write the class to an Archive
//

#pragma once
#include <unordered_map>
#include <utility>
#include <memory>
#include <string>
#include <vector>

// Fetch.ai
#include "serialize.h"
#include "uuid.h"
#include "schema.h"
#include "sessionMessages.h"

// Convert a class that can 'serialise' to a JSON string
template <typename T>
std::string toJsonString(const T &v)
{
  // Create an empty Archive object that will manipulate our buffer
  rapidjson::StringBuffer sb;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer{sb};
  JSONOutputArchive<rapidjson::PrettyWriter<rapidjson::StringBuffer>> ar{writer};

  // Fill our Archive with the object (note an Archive can't be modified after construction)
  v.serialize(ar);

  // Convert to string
  return sb.GetString();
}

// Convert a string to a particular Class, this is the core of JSON parsing on the receive side
//
// usage: std::string result = ...
//        auto d = fromJsonString<QueryAnswer>(result);
template <typename T>
T fromJsonString(const std::string &v)
{
  rapidjson::StringStream s(v.c_str());
  JSONInputArchive js{s};
  return T{js};
}

////////////////////////////////////////////////////////////////////////////////////////
// Basic message classes used when connecting

// Id message, used when first connecting to a Node
class Id
{
 public:
  explicit Id(const std::string &id) : _id{id} {}

  template <typename Archive>
  explicit Id(const Archive &ar) : _id{ar.getString("ID")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ObjectWrapper<Archive> o{ar};
    ar.write("ID", _id);
  }

  std::string id() const { return _id; }

 private:
  std::string _id;
};

// Phrase class, used when handshaking with a Node
class Phrase
{
 public:
  explicit Phrase(const std::string &phrase = "RandomlyGeneratedString") : _phrase{phrase} {}

  template <typename Archive>
  explicit Phrase(const Archive &ar) : _phrase{ar.getString("phrase")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ObjectWrapper<Archive> o{ar};
    ar.write("phrase", _phrase);
  }

  std::string phrase() const { return _phrase; }

 private:
  std::string _phrase;
};

// Used when responding to a 'phrase' in a handshake
class Answer
{
 public:
  explicit Answer(const std::string &answer) : _answer{answer} {}

  template <typename Archive>
  explicit Answer(const Archive &ar) : _answer{ar.getString("answer")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ObjectWrapper<Archive> o{ar};
    ar.write("answer", _answer);
  }

  std::string answer() const { return _answer; }

 private:
  std::string _answer;
};

// Connected class, used by the Node to inform an AEA whether it has successfully connected
class Connected
{
 public:
  explicit Connected(bool status) : _status{status} {}

  template <typename Archive>
  explicit Connected(const Archive &ar) : _status{ar.getBool("connected")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ObjectWrapper<Archive> o{ar};
    ar.write("connected", _status);
  }

  bool status() const { return _status; }

 private:
  bool _status;
};

// Registered will tell an AEA whether it has successfully registered its Instance
class Registered
{
 public:
  explicit Registered(bool status) : _status{status} {}

  template <typename Archive>
  explicit Registered(const Archive &ar) : _status{ar.getBool("registered")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ObjectWrapper<Archive> o{ar};
    ar.write("registered", _status);
  }

  bool status() const { return _status; }

 private:
  bool _status;
};

// When returning from a query, the Node will return a list of agents
class QueryAnswer
{
 public:
  explicit QueryAnswer(const std::vector<std::string> &agents) : _agents{agents} {}

  template <typename Archive>
  explicit QueryAnswer(const Archive &ar)
  {
    ar.template getObjects<std::string>("agents", std::back_inserter(_agents));
  }

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ObjectWrapper<Archive> o{ar};
    ar.write("agents", std::begin(_agents), std::end(_agents));
  }

  std::vector<std::string> agents() const { return _agents; }

 private:
  std::vector<std::string> _agents;
};


////////////////////////////////////////////////////////////////////////////////////////
// More complex message classes that are built up to form an 'Envelope'

class MessageBase
{
 public:
  virtual ~MessageBase() {}
};

class AgentDescription : public MessageBase
{
 public:
  explicit AgentDescription(const Instance &description) : _description{description} {}

  template <typename Archive>
  explicit AgentDescription(const Archive &ar) : _description{ar.getObject("description")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ar.write("description", _description);
  }

  Instance description() const { return _description; }

 private:
  Instance _description;
};

class AgentSearch : public MessageBase
{
 public:
  explicit AgentSearch(const QueryModel &query) : _query{query} {}

  template <typename Archive>
  explicit AgentSearch(const Archive &ar) : _query{ar.getObject("query")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ar.write("query", _query);
  }

  QueryModel query() const { return _query; }

 private:
  QueryModel _query;
};

class SchemaSearch : public MessageBase
{
 public:
  explicit SchemaSearch(const KeywordLookup &keywords) : _keywords{keywords} {}

  template <typename Archive>
  //explicit SchemaSearch(const Archive &ar) : _keywords{ar.template getObjects<std::string>("keywords", std::back_inserter(_keywords))} {} //TODO: (`HUT`) : delete this

  //explicit SchemaSearch(const Archive &ar) : _keywords{ar.getObjectsVector("description")} {}
  explicit SchemaSearch(const Archive &ar) : _keywords{ar.getObject("description")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ar.write("description", _keywords);
  }

  KeywordLookup keywordLookup() const { return _keywords; }

 private:
  KeywordLookup _keywords;
};

// TODO: (`HUT`) : rename this. This is the wrapper for messages between AEAs via a Node
// Note that it has a destination and content, the Node does not know what the content is,
// it just forwards it to the AEA
class Message : public MessageBase
{
 public:
  explicit Message(const Uuid &conversationId, const std::string &destination, const std::string &content) :
    _conversationId{conversationId},
    _destination{destination},
    _content{content} {}

  template <typename Archive>
  explicit Message(const Archive &ar) :
    _conversationId{ar},
    _destination{ar.getString("destination")},
    _content{ar.getString("content")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    _conversationId.serialize(ar);
    ar.write("destination", _destination);
    ar.write("content", _content);
  }

  Uuid uuid() const               { return _conversationId; }
  std::string destination() const { return _destination; }
  std::string content() const     { return _content; }

 private:
  Uuid        _conversationId;
  std::string _destination;
  std::string _content;
};

// The Node will convert a Message to an AgentMessage before forwarding to an AEA so the AEA
// will see it as coming from the OEF
class AgentMessage
{
 public:
  explicit AgentMessage(const Uuid &conversationId, const std::string &origin, const std::string &content) :
    _conversationId{conversationId},
    _origin{origin},
    _content{content} {}

  template <typename Archive>
  explicit AgentMessage(const Archive &ar) :
    _conversationId{ar},
    _origin{ar.getString("origin")},
    _content{ar.getString("content")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ObjectWrapper<Archive> o{ar};
    _conversationId.serialize(ar);
    ar.write("origin", _origin);
    ar.write("content", _content);
  }

  std::string origin() const { return _origin; }
  std::string content() const { return _content; }

 private:
  Uuid        _conversationId;
  std::string _origin;
  std::string _content;
};

// 'Delivered' message the Node will return to an AEA to let it know the message has been sent to another AEA
class Delivered
{
 public:
  explicit Delivered(const Uuid &conversationId, bool status) : _conversationId{conversationId}, _status{status} {}

  template <typename Archive>
  explicit Delivered(const Archive &ar) : _conversationId{ar},  _status{ar.getBool("delivered")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ObjectWrapper<Archive> o{ar};
    _conversationId.serialize(ar);
    ar.write("delivered", _status);
  }

  bool status() const { return _status; }

 private:
  Uuid _conversationId;
  bool _status;
};

// Envelopes 'wrap' all the functionality we expect to execute on a Node
class Envelope
{
 public:
  enum class MsgTypes
  {
    Query, KeywordLookup, Register, Message, Unregister, Description, Search, Error
  };

  explicit Envelope(MsgTypes type, std::unique_ptr<MessageBase> msg) : _type{type}, _message{std::move(msg)} {}

  MsgTypes getType() const { return _type; }

  // Functions for building an Envelope
  static Envelope makeQuery(const QueryModel &query)
  { return Envelope{MsgTypes::Query, std::unique_ptr<AgentSearch>{new AgentSearch{query}}}; }

  static Envelope makeKeywordLookup(const KeywordLookup &keywords)
  { return Envelope{MsgTypes::KeywordLookup, std::unique_ptr<SchemaSearch>{new SchemaSearch{keywords}}}; }

  static Envelope makeRegister(const Instance &description)
  { return Envelope{MsgTypes::Register, std::unique_ptr<AgentDescription>{new AgentDescription{description}}}; }

  static Envelope makeUnregister(const Instance &description)
  { return Envelope{MsgTypes::Unregister, std::unique_ptr<AgentDescription>{new AgentDescription{description}}}; }

  static Envelope makeDescription(const Instance &description)
  { return Envelope{MsgTypes::Description, std::unique_ptr<AgentDescription>{new AgentDescription{description}}}; }

  static Envelope makeSearch(const QueryModel &query)
  { return Envelope{MsgTypes::Search, std::unique_ptr<AgentSearch>{new AgentSearch{query}}}; }

  static Envelope makeMessage(const Uuid &uuid, const std::string &destination, const std::string &content)
  { return Envelope{MsgTypes::Message, std::unique_ptr<Message>{new Message{uuid, destination, content}}}; }

  MessageBase *getMessage() const { return _message.get(); }

  template <typename Archive>
    explicit Envelope(const Archive &ar) : _type{stringToMsgType(ar.getString("type"))}
  {
    switch (_type)
    {
      case MsgTypes::Query:
        _message = std::unique_ptr<AgentSearch>(new AgentSearch(ar));
        break;
      case MsgTypes::Register:
        _message = std::unique_ptr<AgentDescription>(new AgentDescription(ar));
        break;
      case MsgTypes::Message:
        _message = std::unique_ptr<Message>(new Message(ar));
        break;
      case MsgTypes::Unregister:
        _message = std::unique_ptr<AgentDescription>(new AgentDescription(ar));
        break;
      case MsgTypes::Description:
        _message = std::unique_ptr<AgentDescription>(new AgentDescription(ar));
        break;
      case MsgTypes::Search:
        _message = std::unique_ptr<AgentSearch>(new AgentSearch(ar));
        break;
      case MsgTypes::KeywordLookup:
        _message = std::unique_ptr<SchemaSearch>(new SchemaSearch(ar));
        break;
      case MsgTypes::Error:
        break;
    }
  }

  template <typename Archive>
    void serialize(Archive &ar) const
    {
      ObjectWrapper<Archive> o{ar};
      ar.write("type", msgTypeToString(_type));
      switch (_type)
      {
        case MsgTypes::Query:
          {
            AgentSearch *q = dynamic_cast<AgentSearch*>(_message.get());
            q->serialize(ar);
          }
          break;
        case MsgTypes::Register:
          {
            AgentDescription *q = dynamic_cast<AgentDescription*>(_message.get());
            q->serialize(ar);
          }
          break;
        case MsgTypes::Message:
          {
            Message *q = dynamic_cast<Message*>(_message.get());
            q->serialize(ar);
          }
          break;
        case MsgTypes::Unregister:
          {
            AgentDescription *q = dynamic_cast<AgentDescription*>(_message.get());
            q->serialize(ar);
          }
          break;
        case MsgTypes::Description:
          {
            AgentDescription *q = dynamic_cast<AgentDescription*>(_message.get());
            q->serialize(ar);
          }
          break;
        case MsgTypes::Search:
          {
            AgentSearch *q = dynamic_cast<AgentSearch*>(_message.get());
            q->serialize(ar);
          }
          break;
        case MsgTypes::KeywordLookup:
          {
            SchemaSearch *q = dynamic_cast<SchemaSearch*>(_message.get());
            q->serialize(ar);
          }
          break;
        case MsgTypes::Error:
          break;
      }
    }

 private:
  MsgTypes _type;
  std::unique_ptr<MessageBase> _message;
  static std::string msgTypeToString(MsgTypes t)
  {
    switch (t)
    {
      case MsgTypes::Query: return "query"; break;
      case MsgTypes::Register: return "register"; break;
      case MsgTypes::Message: return "message"; break;
      case MsgTypes::Unregister: return "unregister"; break;
      case MsgTypes::Description: return "description"; break;
      case MsgTypes::Search: return "search"; break;
      case MsgTypes::KeywordLookup: return "keywordLookup"; break;
      case MsgTypes::Error: return "error"; break;
    }
    return "error";
  }
  static MsgTypes stringToMsgType(const std::string &s)
  {
    static std::unordered_map<std::string, MsgTypes> types =
    {
      { "query", MsgTypes::Query},
      { "register", MsgTypes::Register},
      { "message", MsgTypes::Message},
      { "unregister", MsgTypes::Unregister},
      { "description", MsgTypes::Description},
      { "search", MsgTypes::Search},
      { "keywordLookup", MsgTypes::KeywordLookup}
    };
    auto iter = types.find(s);
    if (iter == types.end())
      return MsgTypes::Error;
    return iter->second;
  }
};


////////////////////////////////////////////////////////////////////////////////////////
// Temporary classes created for the meteostation demo

class Price
{
 public:
  Price() {}
  explicit Price(float price) : _price{price} {}

  template <typename Archive>
  explicit Price(const Archive &ar) : _price{ar.getFloat("price")} {}

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ObjectWrapper<Archive> o{ar};
    ar.write("price", _price);
  }

  float price() const { return _price; }

 private:
  float _price;
};

class Data
{
 public:
  Data() {}
  explicit Data(const std::string &name, const std::string &type, const std::vector<std::string> &values) :
    _name{name},
    _type{type},
    _values{values} {}

  template <typename Archive>
  explicit Data(const Archive &ar) : _name{ar.getString("name")}, _type{ar.getString("type")}
  {
    ar.template getObjects<std::string>("values", std::back_inserter(_values));
  }

  template <typename Archive>
  void serialize(Archive &ar) const
  {
    ObjectWrapper<Archive> o{ar};
    ar.write("name", _name);
    ar.write("type", _type);
    ar.write("values", std::begin(_values), std::end(_values));
  }

  std::string name() const { return _name; }
  std::string type() const { return _type; }
  std::vector<std::string> values() const { return _values; }

 private:
  std::string              _name;
  std::string              _type;
  std::vector<std::string> _values;
};

class Accepted
{
 public:
  Accepted() {}
  explicit Accepted(bool status) : _status{status} {}

  template <typename Archive>
  explicit Accepted(const Archive &ar) : _status{ar.getBool("accepted")} {}

  template <typename Archive>
    void serialize(Archive &ar) const
    {
      ObjectWrapper<Archive> o{ar};
      ar.write("accepted", _status);
    }
  bool status() const { return _status; }

 private:
    bool _status;
};

///////////////////////////////////////////////////////////////
// new serialization testing
class PriceTest
{
 public:
  PriceTest() {}
  explicit PriceTest(float price, float accuracy) :
    _accuracy{accuracy},
    _price{price} {}

  float price()    const { return _price; }
  float accuracy() const { return _accuracy; }

 //private:
  float _price;
  float _accuracy;
};

template< typename T>
void Serialize( T & serializer, PriceTest const &b) {
  std::cerr << "try this" << std::endl;
  serializer <<  b._price << b._accuracy;
  std::cerr << "tried this" << std::endl;
}

template< typename T>
void Deserialize( T & serializer, PriceTest &b) {
  serializer >> b._price >> b._accuracy;
}
