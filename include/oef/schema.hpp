#ifndef SCHEMA_DEFINES_HPP
#define SCHEMA_DEFINES_HPP

#include<mutex>
#include<set>
#include<unordered_set>
#include<unordered_map>

// TODO: (`HUT`) : probably remove these includes with time
#include<iostream>
#include<experimental/optional>
#include<mapbox/variant.hpp>

#include"json/document.hpp"

// Schema for the OEF: effectively defining the class structure that builds DataModels, Instances and Queries for those Instances

namespace stde = std::experimental; // used for std::optional TODO: (`HUT`) : remove, experimental not guaranteed portable
namespace var = mapbox::util;       // for the variant

namespace fetch
{
namespace schema
{

//helper, variant to string // TODO: (`HUT`) : make this part of variant
std::string vtos(script::Variant var) {
  std::ostringstream ret;
  ret << var;
  return ret.str();
}

// TODO: (`HUT`) : make Type its own class and manage these conversions
enum class Type { Float, Int, Bool, String };
using VariantType = var::variant<int,float,std::string,bool>;
VariantType string_to_value(Type t, const std::string &s);

Type string_to_type(const std::string &s) {
  if(s == "float")
    return Type::Float;
  if(s == "int")
    return Type::Int;
  if(s == "bool")
    return Type::Bool;
  if(s == "string")
    return Type::String;
  throw std::invalid_argument(s + std::string(" is not a valid type"));
}

std::string type_to_string(Type t) {
  switch(t) {
    case Type::Float:  return "float";
    case Type::Int:    return "int";
    case Type::Bool:   return "bool";
    case Type::String: return "string";
  }
  return "";
}

class Attribute {
public:
  explicit Attribute() {}
  explicit Attribute(const std::string &name, Type type, bool required, stde::optional<std::string> description = stde::nullopt) :
    name_{name}, type_{type}, required_{required}, description_{description} {}

  Attribute(fetch::json::JSONDocument &jsonDoc) {
    LOG_STACK_TRACE_POINT;
    name_     = std::string(jsonDoc["name"].as_byte_array());
    type_     = string_to_type(std::string(jsonDoc["type"].as_byte_array()));
    required_ = jsonDoc["required"].as_bool();
  }

  fetch::script::Variant variant() const {
    fetch::script::Variant result = fetch::script::Variant::Object();
    result["name"]                = name_;
    result["type"]                = type_to_string(type_);
    result["required"]            = required_;
    return result;
  }

  std::pair<std::string,std::string> instantiate(const std::unordered_map<std::string,std::string> &values) const {
    auto iter = values.find(name_);
    if(iter == values.end()) {
      if(required_) {
        throw std::invalid_argument(std::string("Missing value: ") + name_);
      }
      return std::make_pair(name_,"");
    }
    if(validate(iter->second)) {
      return std::make_pair(name_, iter->second);
    }
    throw std::invalid_argument(name_ + std::string(" has a wrong type of value ") + iter->second);
  }

  const std::string &name() const     { return name_; }
  std::string       &name()           { return name_; }
  const bool        &required() const { return required_; }
  bool              &required()       { return required_; }
  const Type        &type() const     { return type_; }
  Type              &type()           { return type_; }

private:
  std::string                 name_;
  Type                        type_;
  bool                        required_;
  stde::optional<std::string> description_;

  bool validate(const std::string &value) const {
    try {
      switch(type_) {
      case Type::Float:
        (void)std::stod(value);
        break;
      case Type::Int:
        (void)std::stol(value);
        break;
      case Type::Bool:
        return value == "true" || value == "false" || value == "1" || value == "0";
        break;
      case Type::String:
        return true;
        break;
      }
    } catch(std::exception &e) {
      return false;
    }
    return true;
  }
};

std::string type_to_string(var::variant<int,float,std::string,bool> value) {

  std::string type = "FAIL";

  value.match(
          [&] (int &a)         { type = "int"; },
          [&] (float &a)       { type = "float"; },
          [&] (std::string &a) { type = "string"; },
          [&] (bool &a)        { type = "bool"; }
          );

  return type;
}

std::string to_string(var::variant<int,float,std::string,bool> value) {

  std::string type = "FAILED";

  value.match(
          [&] (int &a)         { type = std::to_string(a); },
          [&] (float &a)       { type = std::to_string(a); },
          [&] (std::string &a) { type = a; },
          [&] (bool &a)        { type = std::to_string(a); }
          );

  return type;
}

class Relation {
public:
  using ValueType = var::variant<int,float,std::string,bool>;
  enum class Op { Eq, Lt, Gt, LtEq, GtEq, NotEq };

  Relation() {}
  explicit Relation(Op op, const ValueType &value) : op_{op}, value_{value} {}

  static std::string op_to_string(Op op) {
    switch(op) {
    case Op::Eq:    return "=";
    case Op::Lt:    return "<";
    case Op::LtEq:  return "<=";
    case Op::Gt:    return ">";
    case Op::GtEq:  return ">=";
    case Op::NotEq: return "<>";
    }
    return "";
  }

  static Op string_to_op(const std::string &s) {
    if(s == "=")  return Op::Eq;
    if(s == "<")  return Op::Lt;
    if(s == "<=") return Op::LtEq;
    if(s == ">")  return Op::Gt;
    if(s == ">=") return Op::GtEq;
    if(s == "<>") return Op::NotEq;
    throw std::invalid_argument(s + std::string{" is not a valid operator."});
  }

  template <typename T>
  bool check_value(const T &v) const {
    auto &s = value_.get<T>();
    switch(op_) {
      case Op::Eq:    return s == v;
      case Op::NotEq: return s != v;
      case Op::Lt:    return v < s;
      case Op::LtEq:  return v <= s;
      case Op::Gt:    return v > s;
      case Op::GtEq:  return v >= s;
    }
  return false;
  }

  bool check(const VariantType &v) const {
    bool res = false;
    v.match([&res,this](int i) {
        res = check_value(i);
      },[&res,this](float f) {
        res = check_value(f);
      },[&res,this](const std::string &s) {
        res = check_value(s);
      },[&res,this](bool b) {
        res = check_value(b);
      });
    return res;
  }

  fetch::script::Variant variant() const {

    fetch::script::Variant result = fetch::script::Variant::Object();

    result["type"]       = "relation";        // TODO: (`HUT`) : fix this
    result["op"]         = op_to_string(op_);
    result["value_type"] = type_to_string(value_);
    result["value"]      = to_string(value_);

    return result;
  }

  const Op        &op() const        { return op_; }
  Op              &op()              { return op_; }
  const ValueType &valueType() const { return value_; }
  ValueType       &valueType()       { return value_; }

private:
  Op        op_;
  ValueType value_;
};

class Set {
public:
  using ValueType = var::variant<std::unordered_set<int>,std::unordered_set<float>, std::unordered_set<std::string>,std::unordered_set<bool>>;
  enum class Op { In, NotIn };

  explicit Set(Op op, const ValueType &values) : op_{op}, values_{values} {}

  bool check(const VariantType &v) const {
    bool res = false;
    v.match([&res,this](int i) {
        auto &s = values_.get<std::unordered_set<int>>();
        res = s.find(i) != s.end();
      },[&res,this](float f) {
        auto &s = values_.get<std::unordered_set<float>>();
        res = s.find(f) != s.end();
      },[&res,this](const std::string &st) {
        auto &s = values_.get<std::unordered_set<std::string>>();
        res = s.find(st) != s.end();
      },[&res,this](bool b) {
        auto &s = values_.get<std::unordered_set<bool>>();
        res = s.find(b) != s.end();
      });
    return res;
  }

  const Op        &op() const         { return op_; }
  Op              &op()               { return op_; }
  const ValueType &valueTypes() const { return values_; }
  ValueType       &valueTypes()       { return values_; }

private:
  Op        op_;
  ValueType values_;

  std::string op_to_string(Op op) const {
    switch(op) {
      case Op::In: return "in";
      case Op::NotIn: return "not in";
    }
    return "";
  }

  Op string_to_op(const std::string &s) const {
    if(s == "in") return Op::In;
    if(s == "not in") return Op::NotIn;
    throw std::invalid_argument(s + std::string{" is not a valid operator."});
  }
};

class Range {
public:
  using ValueType = var::variant<std::pair<int,int>,std::pair<float,float>,std::pair<std::string,std::string>>;
  explicit Range(ValueType v) : pair_{v} {}
  // enable_if is necessary to prevent both constructor to conflict.
  //
  bool check(const VariantType &v) const {
    bool res = false;
    v.match([&res,this](int i) {
        auto &p = pair_.get<std::pair<int,int>>();
        res = i >= p.first && i <= p.second;
      },[&res,this](float f) {
        auto &p = pair_.get<std::pair<float,float>>();
        res = f >= p.first && f <= p.second;
      },[&res,this](const std::string &s) {
        auto &p = pair_.get<std::pair<std::string,std::string>>();
        res = s >= p.first && s <= p.second;
      },[&res,this](bool b) {
        res = false; // doesn't make sense
      });
    return res;
  }

private:
  ValueType pair_;
};

class DataModel {
public:
  explicit DataModel() {}

  explicit DataModel(const std::string &name, const std::vector<Attribute> &attributes) :
    name_{name},
    attributes_{attributes} {}

  DataModel(fetch::json::JSONDocument jsonDoc) {
    LOG_STACK_TRACE_POINT;

    name_ = std::string(jsonDoc["name"].as_byte_array());

    for(auto &a: jsonDoc["attributes"].as_array()) {

      fetch::json::JSONDocument doc; // TODO: (`HUT`) : create a fix for this
      doc.root() = a;

      Attribute attribute(doc);
      attributes_.push_back(attribute);
    }

    for(auto &a: jsonDoc["keywords"].as_array()) {
      keywords_.push_back(a.as_byte_array());
    }
  }

  // nhutton: Temporary keyword testing TODO: (`HUT`) : put in constructor
  void addKeywords(std::vector<std::string> keywords) {
    for(auto i : keywords) {
      keywords_.push_back(i);
    }
  }

  bool operator==(const DataModel &other) const
  {
    if(name_ != other.name_)
      return false;
    // TODO: should check more.
    return true;
  }

  stde::optional<Attribute> attribute(const std::string &name) const
  {
    for(auto &a : attributes_) {
      if(a.name() == name)
        return stde::optional<Attribute>{a};
    }
    return stde::nullopt;
  }

  std::vector<std::pair<std::string,std::string>> instantiate(const std::unordered_map<std::string,std::string> &values) const
  {
    std::vector<std::pair<std::string,std::string>> res;
    for(auto &a : attributes_)
    {
      res.emplace_back(a.instantiate(values));
    }
    return res;
  }

  const std::string              &name() const       { return name_; }
  std::string                    &name()             { return name_; }
  const std::vector<std::string> &keywords() const   { return keywords_; }
  std::vector<std::string>       &keywords()         { return keywords_; }
  const std::vector<Attribute>   &attributes() const { return attributes_; }
  std::vector<Attribute>         &attributes()       { return attributes_; }

private:
  std::string                 name_;
  std::vector<Attribute>      attributes_;
  std::vector<std::string>    keywords_;
  stde::optional<std::string> description_;  // TODO: (`HUT`) : consider whether we want to use descriptions
};

class Instance {
public:
  Instance() {}

  Instance(const DataModel &model, const std::unordered_map<std::string,std::string> &values)
    : model_{model}, values_{values} {}

  Instance(fetch::json::JSONDocument jsonDoc)
    : model_{jsonDoc["dataModel"]} {
    LOG_STACK_TRACE_POINT;

    for(auto &a: jsonDoc["values"].as_array()) {
      for(auto &b: a.as_dictionary()) {
        std::string first(b.first);
        std::string second(b.second.as_byte_array());
        values_[first] = second;
      }
    }
  }

  fetch::script::Variant variant() const {
    fetch::script::Variant result = fetch::script::Variant::Object();

    //result["dataModel"] = model_.variant(); // TODO: (`HUT`) : fill in this functionality
    result["values"]    = fetch::script::Variant::Array(values_.size());

    int index = 0;
    for(auto &val : values_) {
      fetch::script::Variant value = fetch::script::Variant::Object();
      value[val.first] = val.second;
      result["values"][index]    = value;
      index++;
    }

    return result;
  }

  bool operator==(const Instance &other) const
  {
    if(!(model_ == other.model_))
      return false;
    for(const auto &p : values_) {
      const auto &iter = other.values_.find(p.first);
      if(iter == other.values_.end())
        return false;
      if(iter->second != p.second)
        return false;
    }
    return true;
  }

  std::size_t hash() const {
    std::size_t h = std::hash<std::string>{}(model_.name());
    for(const auto &p : values_) {
      std::size_t hs = std::hash<std::string>{}(p.first);
      h = hs ^ (h << 1);
      hs = std::hash<std::string>{}(p.second);
      h = hs ^ (h << 2);
    }
    return h;
  }

  std::vector<std::pair<std::string,std::string>>
    instantiate() const {
    return model_.instantiate(values_);
  }

  stde::optional<std::string> value(const std::string &name) const {
    auto iter = values_.find(name);
    if(iter == values_.end())
      return stde::nullopt;
    return stde::optional<std::string>{iter->second};
  }

  DataModel model() const { // TODO: (`HUT`) : delete
    return model_;
  }

  const std::unordered_map<std::string,std::string> &values() const    { return values_; }
  std::unordered_map<std::string,std::string>       &values()          { return values_; }
  const DataModel                                   &dataModel() const { return model_; }
  DataModel                                         &dataModel()       { return model_; }

private:
  DataModel                                   model_;
  std::unordered_map<std::string,std::string> values_;
};

class Or;
class And;

class ConstraintType {
public:
  using ValueType = var::variant<var::recursive_wrapper<Or>,var::recursive_wrapper<And>,Range,Relation,Set>; // TODO: (`HUT`) : return set
  explicit ConstraintType() {}
  explicit ConstraintType(ValueType v) : constraint_{v} {}

  ConstraintType(fetch::json::JSONDocument &jsonDoc) {
    LOG_STACK_TRACE_POINT;

    // TODO: (`HUT`) : all possible types
    std::string type = jsonDoc["type"].as_byte_array();

    if(type.compare("relation") == 0) {

      // TODO: (`HUT`) : make this cleaner
      if(jsonDoc["value"].is_bool()) {
        constraint_ = Relation(Relation::string_to_op(jsonDoc["op"].as_byte_array()), jsonDoc["value"].as_bool());
      } else if(jsonDoc["value"].is_int()) {
        constraint_ = Relation(Relation::string_to_op(jsonDoc["op"].as_byte_array()), int(jsonDoc["value"].as_int()));
      } else if(jsonDoc["value"].is_float()) {
        constraint_ = Relation(Relation::string_to_op(jsonDoc["op"].as_byte_array()), float(jsonDoc["value"].as_double()));
      } else if(jsonDoc["value"].is_byte_array()) {
        constraint_ = Relation(Relation::string_to_op(jsonDoc["op"].as_byte_array()), std::string(jsonDoc["value"].as_byte_array()));
      } else {
        throw std::invalid_argument(std::string("Incorrect attempt to parse ConstraintType due to missing functionality!"));
      }
    } else {
      throw std::invalid_argument(std::string("Incorrect attempt to parse ConstraintType due to missing functionality - not a relation!"));
    }
  }

  fetch::script::Variant to_variant() const {
    Relation rel = mapbox::util::get<Relation>(constraint_);
    return rel.variant();
  }

  bool check(const VariantType &v) const;

  const ConstraintType::ValueType& constraint() const { return constraint_; }
  ConstraintType::ValueType& constraint()             { return constraint_; }

private:
  ConstraintType::ValueType constraint_;
};

class Constraint {
public:
  explicit Constraint() {}

  explicit Constraint(const Attribute &attribute, const ConstraintType &constraint)
    : attribute_{attribute}, constraint_{constraint} {}

  Constraint(fetch::json::JSONDocument &jsonDoc) {
    LOG_STACK_TRACE_POINT;

    fetch::json::JSONDocument doc;
    doc.root() = jsonDoc["attribute"];

    Attribute attribute(doc);
    attribute_ = attribute;

    doc.root() = jsonDoc["constraint"];
    ConstraintType constraintType(doc);
    constraint_ = constraintType;
  }

  fetch::script::Variant variant() const {
    fetch::script::Variant result = fetch::script::Variant::Object();
    result["constraint"]          = constraint_.to_variant();
    result["attribute"]           = attribute_.variant();
    return result;
  }

  bool check(const VariantType &v) const {
    return constraint_.check(v);
  }

  bool check(const Instance &instance) const {
    const auto &model = instance.model();
    auto attr = model.attribute(attribute_.name());
    if(attr) {
      if(attr->type() != attribute_.type()) { // TODO: (`HUT`) : fix this by removin std::opt
        std::cout << "HERE" << std::endl;
        return false;
      }
    }
    auto v = instance.value(attribute_.name());
    if(!v) {
      if(attribute_.required()) {
        std::cerr << "Should not happen!\n"; // Exception ?
      }
      std::cout << "HERE2" << std::endl;
      return false;
    }
    VariantType value{string_to_value(attribute_.type(), *v)};
    return check(value);
  }

  const Attribute      &attribute() const      { return attribute_; }
  Attribute            &attribute()            { return attribute_; }
  const ConstraintType &constraintType() const { return constraint_; }
  ConstraintType       &constraintType()       { return constraint_; }

private:
  Attribute      attribute_;
  ConstraintType constraint_;
};

class Or {
  std::vector<ConstraintType> expr_;
public:
  explicit Or(const std::vector<ConstraintType> expr) : expr_{expr} {}
  explicit Or() {}; // to make happy variant.

  bool check(const VariantType &v) const {
    for(auto &c : expr_) {
      if(c.check(v))
        return true;
    }
    return false;
  }
};

class And {
private:
  std::vector<ConstraintType> expr_;
public:
  explicit And(const std::vector<ConstraintType> expr) : expr_{expr} {}

  bool check(const VariantType &v) const {
    for(auto &c : expr_) {
      if(!c.check(v))
        return false;
    }
    return true;
  }

  const std::vector<ConstraintType> &expressions() const { return expr_; }
  std::vector<ConstraintType>       &expressions()       { return expr_; }
};

class QueryModel {
public:

  explicit QueryModel() : model_{stde::nullopt} {}

  explicit QueryModel(const std::vector<Constraint> &constraints, stde::optional<DataModel> model = stde::nullopt)
    : constraints_{constraints}, model_{model} {}

  QueryModel(fetch::json::JSONDocument &jsonDoc) {
    LOG_STACK_TRACE_POINT;

    for(auto &a: jsonDoc["constraints"].as_array()) {

      fetch::json::JSONDocument doc; // TODO: (`HUT`) : create a fix for this
      doc.root() = a;

      Constraint constraint(doc);
      constraints_.push_back(constraint);
    }

    if(!jsonDoc["keywords"].is_undefined()) {
      for(auto &a: jsonDoc["keywords"].as_array()) {
        keywords_.push_back(a.as_byte_array());
      }
    }
  }

  // TODO: (`HUT`) : add keywords to variant
  fetch::script::Variant variant() const {
    fetch::script::Variant result = fetch::script::Variant::Array(constraints_.size());

    for (int i = 0; i < constraints_.size(); ++i) {
      result[i] = constraints_[i].variant();
    }

    return result;
  }

  template <typename T>
    bool check_value(const T &v) const {
    for(auto &c : constraints_) {
      if(!c.check(VariantType{v}))
        return false;
    }
    return true;
  }

  bool check(const Instance &i) const {

    if(model_) {
      if(model_->name() != i.model().name()) {
        return false;
      }
      // TODO: more to compare ?
    }

    for(auto &c : constraints_) {
      if(!c.check(i)) {
        return false;
      }
    }
    return true;
  }

  // TODO: (`HUT`) : think about model comparison
  bool operator==(const QueryModel &rhs) const {
    return (vtos(this->variant())).compare(vtos(rhs.variant())) == 0 && // TODO: (`HUT`) : variant to string comparison is dangerous, change this
           hash_ == rhs.hash() &&
           //std::sort(keywords_) == std::sort(rhs.keywords());
           1 == 1; // TODO: (`HUT`) : keyword sort and compare
  }

  const std::vector<Constraint>  &constraints() const { return constraints_; }
  std::vector<Constraint>        &constraints()       { return constraints_; }
  const std::vector<std::string> &keywords() const    { return keywords_; }
  std::vector<std::string>       &keywords()          { return keywords_; }
  const uint64_t                 &hash() const        { return hash_; }
  uint64_t                       &hash()              { return hash_; }

private:
  std::vector<Constraint>   constraints_;
  std::vector<std::string>  keywords_;
  stde::optional<DataModel> model_; // TODO: (`HUT`) : this is not serialized yet, nor JSON-ed
  uint64_t                  hash_ = static_cast<uint64_t>(time(NULL));
};

class QueryModelMulti {

public:
  explicit QueryModelMulti() {}

  explicit QueryModelMulti(const QueryModel &aeaQuery, const QueryModel &forwardingQuery, uint16_t jumps=3)
    : aeaQuery_{aeaQuery}, forwardingQuery_{forwardingQuery}, jumps_{jumps} {}

  QueryModelMulti& operator--(int) {
    if(jumps_ > 0) {
      jumps_--;
    }
    return *this;
  }

  // TODO: (`HUT`) : more robust comparison than this
  bool operator< (const QueryModelMulti &rhs) const {
      return (hash_ < rhs.hash());
  }

  // Note: do not compare jumps
  bool operator==(const QueryModelMulti &rhs) const {
    return aeaQuery_ == rhs.aeaQuery() &&
           forwardingQuery_ == rhs.forwardingQuery() &&
           hash_ == rhs.hash();
  }

  fetch::script::Variant variant() const {
    fetch::script::Variant result = fetch::script::Variant::Object();
    result["aeaQuery"]        = aeaQuery_.variant();
    result["forwardingQuery"] = forwardingQuery_.variant();
    return result;
  }

  const QueryModel &aeaQuery() const        { return aeaQuery_; }
  QueryModel       &aeaQuery()              { return aeaQuery_; }
  const QueryModel &forwardingQuery() const { return forwardingQuery_; }
  QueryModel       &forwardingQuery()       { return forwardingQuery_; }
  const uint32_t   &jumps() const           { return jumps_; }
  uint32_t         &jumps()                 { return jumps_; }
  const uint64_t   &hash() const            { return hash_; }
  uint64_t         &hash()                  { return hash_; }

private:
  //uint32_t   &jumps()                       { return jumps_; }
  QueryModel aeaQuery_;
  QueryModel forwardingQuery_;
  uint32_t   jumps_;
  uint64_t   hash_ = static_cast<uint64_t>(time(NULL));
};

// Temporarily place convenience fns here
bool ConstraintType::check(const VariantType &v) const {
  bool res = false;
  constraint_.match([&v,&res](const Range &r) {
      res = r.check(v);
    },[&v,&res](const Relation &r) {
      res =  r.check(v);
    },[&v,&res](const Set &r) {
      res = r.check(v);
    },[&v,&res](const Or &r) {
      res = r.check(v);
    },[&v,&res](const And &r) {
      res = r.check(v);
    });
  return res;
}

VariantType string_to_value(Type t, const std::string &s) {
  switch(t) {
  case Type::Float:
    return VariantType{float(std::stod(s))};
  case Type::Int:
    return VariantType{int(std::stol(s))};
  case Type::String:
    return VariantType{s};
  case Type::Bool:
    return VariantType{s == "1" || s == "true"};
  }
  // should not reach this line
  return VariantType{std::string{""}};
}

// Used for managing node to node communications
class Endpoint {
public:
  Endpoint() {}

  Endpoint(const std::string &IP, const int TCPPort)      : IP_{IP}, TCPPort_{uint16_t(TCPPort)} {}
  Endpoint(const std::string &IP, const uint16_t TCPPort) : IP_{IP}, TCPPort_{TCPPort} {}

  Endpoint(fetch::json::JSONDocument jsonDoc) {
    LOG_STACK_TRACE_POINT;

    IP_      = std::string(jsonDoc["IP"].as_byte_array());
    TCPPort_ = uint16_t(jsonDoc["TCPPort"].as_int());
  }

  bool operator< (const Endpoint &rhs) const {
      return (TCPPort_ < rhs.TCPPort()) || (IP_ < rhs.IP());
  }

  bool equals(const Endpoint &rhs) const {
    return (TCPPort_ == rhs.TCPPort()) && (IP_ == rhs.IP());
  }

  fetch::script::Variant variant() const {
    fetch::script::Variant result = fetch::script::Variant::Object();
    result["IP"]      = IP_;
    result["TCPPort"] = TCPPort_;
    return result;
  }

  const std::string &IP() const      { return IP_; }
  std::string       &IP()            { return IP_; }
  const uint16_t    &TCPPort() const { return TCPPort_; }
  uint16_t          &TCPPort()       { return TCPPort_; }

private:
  std::string IP_;
  uint16_t    TCPPort_;
};

class Endpoints {
public:
  Endpoints() {}

  Endpoints(Endpoint endpoint) {endpoints_.insert(endpoint);}
  explicit Endpoints(const std::set<Endpoint> &endpoints) : endpoints_{endpoints} {}

  Endpoints(fetch::json::JSONDocument jsonDoc) {
    LOG_STACK_TRACE_POINT;

    for(auto &b: jsonDoc.root().as_array()) {
      endpoints_.insert(Endpoint(b));
    }
  }

  fetch::script::Variant variant() const {
    fetch::script::Variant result = fetch::script::Variant::Array(endpoints_.size());

    int index = 0;
    for(auto &i : endpoints_) {
      result[index++] = i.variant();
    }
    return result;
  }

  const std::set<Endpoint> &endpoints() const { return endpoints_; }
  std::set<Endpoint>       &endpoints()       { return endpoints_; }

private:
  std::set<Endpoint> endpoints_;
};

// The Agents class is just a convenience for representing agents, this will be extended to hold agent-specific information later
class Agents {
public:
  explicit Agents() {}
  explicit Agents(const std::string &agent) {Insert(agent);}

  bool Insert(const std::string &agent) {
    return agents_.insert(agent).second;
  }

  bool Erase(const std::string &agent) {
    return agents_.erase(agent) == 1;
  }

  bool Contains(const std::string &agent) {
    return agents_.find(agent) != agents_.end();
  }

  size_t size() const {
    return agents_.size();
  }

  fetch::script::Variant variant() const {

    fetch::script::Variant res = fetch::script::Variant::Array(agents_.size());

    int index = 0;
    for(auto &i : agents_) {
      res[index++] = fetch::script::Variant(i);
    }

    return res;
  }

  void Copy(std::unordered_set<std::string> &s) const {
    std::copy(agents_.begin(), agents_.end(), std::inserter(s, s.end()));
  }

private:
  std::unordered_set<std::string> agents_;
};

}
}

// Used for hashing classes for use in unordered_map etc. // TODO: (`HUT`) : think about how to manage this
namespace std
{
  template<> struct hash<fetch::schema::Instance>  {
    typedef fetch::schema::Instance argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const noexcept
    {
      return s.hash();
    }
  };
}
#endif
