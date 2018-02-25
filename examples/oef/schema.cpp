#include "old_oef_codebase/lib/include/schema.h"

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
	case Type::Float: return "float";
	case Type::Int: return "int";
	case Type::Bool: return "bool";
	case Type::String: return "string";
	}
	return "";
}

std::string t_to_string(int i) { return "int"; }
std::string t_to_string(float f) { return "float"; }
std::string t_to_string(bool b) { return "bool"; }
std::string t_to_string(const std::string &s) { return "string"; }

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

bool ConstraintType::check(const VariantType &v) const {
  bool res = false;
  _constraint.match([&v,&res](const Range &r) {
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

