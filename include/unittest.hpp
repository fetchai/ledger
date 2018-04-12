#ifndef UNITTEST_HPP
#define UNITTEST_HPP

#include "commandline/vt100.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace fetch {
namespace unittest {

enum class UnitTestOutputFormat {
  FORMAT_NOFORMAT = 0,
  FORMAT_HEADING = 1,
  FORMAT_SECTION = 2,
  FORMAT_SUBSECTION = 3,
  FORMAT_TEST = 4
};

class TestContext : public std::enable_shared_from_this<TestContext> {
 public:
  typedef std::shared_ptr<TestContext> self_shared_type;
  typedef std::function<void(self_shared_type)> function_type;
  std::vector<TestContext*> sections;

  TestContext(std::string const& explanation) : explanation_(explanation) {
    TestContext::sections.push_back(this);
  }

  function_type operator=(function_type fnc) {
    function_ = fnc;
    this->operator()();
    return fnc;
  }

  virtual void operator()() {
    if (function_) {
      switch (format_) {
        case UnitTestOutputFormat::FORMAT_NOFORMAT:
          break;
        case UnitTestOutputFormat::FORMAT_HEADING:
          std::cout << std::endl;
          std::cout << commandline::VT100::GetColor("red", "default");
          break;
        case UnitTestOutputFormat::FORMAT_SECTION:
          std::cout << std::endl;
          std::cout << "  "
                    << commandline::VT100::GetColor("yellow", "default");
          break;
        case UnitTestOutputFormat::FORMAT_SUBSECTION:
          std::cout << "    ";
          break;
        case UnitTestOutputFormat::FORMAT_TEST:
          std::cout << "     - ";
          break;
      }
      std::cout << explanation_ << commandline::VT100::DefaultAttributes()
                << std::endl;
      if (format_ == UnitTestOutputFormat::FORMAT_HEADING) {
        for (std::size_t i = 0; i < explanation_.size(); ++i) std::cout << "=";
        std::cout << std::endl;
      }

      function_(this->shared_from_this());
    }
  }

  template <typename T, typename... A>
  self_shared_type NewContext(A... args) {
    self_shared_type ctx = std::make_shared<TestContext>(args...);
    subcontexts_.push_back(ctx);
    return ctx;
  }

  template <typename T>
  TestContext const& operator<<(T const& val) const {
    std::cout << val;
    return *this;
  }

  TestContext const& operator<<(char const* val) const {
    std::cout << val;
    return *this;
  }

  TestContext& operator<<(UnitTestOutputFormat const& val) {
    format_ = val;
    return *this;
  }

  template <typename T>
  TestContext& operator<<(T const& val) {
    std::cout << val;
    return *this;
  }

  TestContext& operator<<(char const* val) {
    std::cout << val;
    return *this;
  }

  self_shared_type last() { return subcontexts_.back(); }

 private:
  std::string explanation_;
  function_type function_;
  std::vector<self_shared_type> subcontexts_;
  UnitTestOutputFormat format_ = UnitTestOutputFormat::FORMAT_NOFORMAT;
};

class Expression {
 public:
  Expression() = default;
  Expression(Expression const& other) = default;
  Expression(Expression&& other) = default;
  Expression(std::string const& expr) : expression_(expr) {}

  template <typename T>
  Expression(T const& v) {
    std::stringstream s;
    s << v;
    expression_ = s.str();
  }

  Expression(std::string const& expr, Expression const& lhs,
             Expression const& rhs)
      : expression_(expr),
        lhs_(new Expression(lhs)),
        rhs_(new Expression(rhs)) {}

#define ADD_OP(OP)                             \
  template <typename T>                        \
  Expression operator OP(T const& rhs_value) { \
    Expression rhs(rhs_value);                 \
    Expression ret(#OP, *this, rhs);           \
    return ret;                                \
  }
  ADD_OP(|)
  ADD_OP(||)
  ADD_OP(&)
  ADD_OP(&&)
  ADD_OP(+)
  ADD_OP(-)
  ADD_OP(*)
  ADD_OP(/)
  ADD_OP(==)
  ADD_OP(!=)
  ADD_OP(<)
  ADD_OP(>)    
#undef ADD_OP

  std::string const& expression() const { return expression_; }
  Expression const* left_hand_side() const { return lhs_; }
  Expression const* right_hand_side() const { return rhs_; }

 private:
  std::string expression_;
  Expression *lhs_ = nullptr, *rhs_ = nullptr;
};

class ExpressionStart {
 public:
  template <typename T>
  Expression operator*(T const& rhs_value) {
    return Expression(rhs_value);
  }
};

std::ostream& operator<<(std::ostream& strm, Expression const& obj) {
  if (obj.left_hand_side() != nullptr) strm << (*obj.left_hand_side());
  if (obj.expression() != "") strm << obj.expression();
  if (obj.right_hand_side() != nullptr) strm << (*obj.right_hand_side());

  return strm;
}

class ProgramInserter {
 public:
  typedef TestContext::self_shared_type shared_context_type;
  typedef std::function<void(shared_context_type)> sub_function_type;
  typedef std::function<void(sub_function_type)> main_function_type;

  ProgramInserter(main_function_type fnc) : main_(fnc) {}

  void operator()() {
    sub_function_type subber = [this](shared_context_type self) {
      if (this->sub_) this->sub_(self);
    };
    if (main_) main_(subber);
  }

  sub_function_type operator=(sub_function_type sub) {
    sub_ = sub;
    this->operator()();
    return sub;
  }

 private:
  main_function_type main_;
  sub_function_type sub_;
};

namespace details {
typedef std::shared_ptr<TestContext> shared_context_type;
std::vector<shared_context_type> unit_tests;
template <typename... A>
shared_context_type NewTest(A... Args) {
  shared_context_type ret = std::make_shared<TestContext>(Args...);
  unit_tests.push_back(ret);
  return ret;
}

typedef std::shared_ptr<ProgramInserter> shared_inserter_type;
std::vector<shared_inserter_type> inserted_programs;
template <typename... A>
shared_inserter_type NewNestedProgram(A... Args) {
  shared_inserter_type ret = std::make_shared<ProgramInserter>(Args...);
  inserted_programs.push_back(ret);
  return ret;
}
ProgramInserter& last_inserter() { return *inserted_programs.back(); }
};

#define SECTION_REF(EXPLANATION)                                        \
  self->NewContext<fetch::unittest::TestContext>(EXPLANATION);          \
  (*self->last()) << fetch::unittest::UnitTestOutputFormat::FORMAT_SECTION; \
  (*self->last()) = [&](                                                \
      fetch::unittest::TestContext::self_shared_type self) mutable

#define SECTION(EXPLANATION)                                                \
  self->NewContext<fetch::unittest::TestContext>(EXPLANATION);              \
  (*self->last()) << fetch::unittest::UnitTestOutputFormat::FORMAT_SECTION; \
  (*self->last()) = [=](fetch::unittest::TestContext::self_shared_type self) \
                     mutable

#define SCENARIO(NAME)                                          \
  fetch::unittest::details::NewTest(NAME);                      \
  (*fetch::unittest::details::unit_tests.back())                \
      << fetch::unittest::UnitTestOutputFormat::FORMAT_HEADING; \
  (*fetch::unittest::details::unit_tests.back()) = [&](          \
      fetch::unittest::TestContext::self_shared_type self)

//#define CURSOR_MOVE(c)                      \
//  c <<

#define CAPTURE(EXPRESSION) \
  (*self) << #EXPRESSION << " := " << EXPRESSION << "\n";

#define INFO(EXPRESSION)                                                    \
  (*self) << "     - " << #EXPRESSION << "\n";                                      


#define EXPECT_FAIL_SUCCESS(EXPRESSION)\
  if (EXPRESSION) {                                                         \
    (*self) << fetch::commandline::VT100::Return                            \
            << fetch::commandline::VT100::Right(70);                        \
    (*self) << " [  " << fetch::commandline::VT100::Bold;                   \
    (*self) << fetch::commandline::VT100::GetColor("yellow", "default");    \
    (*self) << "OK" << fetch::commandline::VT100::DefaultAttributes()       \
            << "  ]\n";                                                     \
  } else {                                                                  \
    (*self) << fetch::commandline::VT100::Return                            \
            << fetch::commandline::VT100::Right(70);                        \
    (*self) << " [ " << fetch::commandline::VT100::Bold;                    \
    (*self) << fetch::commandline::VT100::GetColor("red", "default");       \
    (*self) << "FAIL" << fetch::commandline::VT100::DefaultAttributes()     \
            << " ]\n\n\n";                                                  \
    (*self) << "Expect failed " << __FILE__ << " on line " << __LINE__      \
            << ": \n\n";                                                    \
    (*self) << "    " << #EXPRESSION << "\n\nwhich expands to:\n\n    ";    \
    (*self) << (fetch::unittest::ExpressionStart() * EXPRESSION) << "\n\n\n\n"; \
    exit(-1);                                                               \
  }

#define EXPECT(EXPRESSION)                                                  \
  (*self) << "     - " << #EXPRESSION;                                      \
  EXPECT_FAIL_SUCCESS(EXPRESSION)

#define EXPECT_EXCEPTION(EXPRESSION, EXC)                               \
  (*self) << "     - " << #EXPRESSION << " => throw " << #EXC;                  \
  {                                                                     \
  bool test_success = false;                                            \
  try {                                                                 \
    EXPRESSION;                                                         \
  } catch(EXC const &e) {                                               \
    test_success = true;                                                \
  }                                                                     \
  if (test_success) {                                                         \
    (*self) << fetch::commandline::VT100::Return                            \
            << fetch::commandline::VT100::Right(70);                        \
    (*self) << " [  " << fetch::commandline::VT100::Bold;                   \
    (*self) << fetch::commandline::VT100::GetColor("yellow", "default");    \
    (*self) << "OK" << fetch::commandline::VT100::DefaultAttributes()       \
            << "  ]\n";                                                     \
  } else {                                                                  \
    (*self) << fetch::commandline::VT100::Return                            \
            << fetch::commandline::VT100::Right(70);                        \
    (*self) << " [ " << fetch::commandline::VT100::Bold;                    \
    (*self) << fetch::commandline::VT100::GetColor("red", "default");       \
    (*self) << "FAIL" << fetch::commandline::VT100::DefaultAttributes()     \
            << " ]\n\n\n";                                                  \
    (*self) << "Expect failed " << __FILE__ << " on line " << __LINE__      \
            << ": \n\n";                                                    \
    (*self) << "    " << #EXC << "was never thrown    " << "\n\n"; \
    exit(-1);                                                               \
  }                                                                     \
  }
  

  
#define CHECK(TEXT, EXPRESSION)                                             \
  (*self) << "     - " << TEXT;                                             \
  EXPECT_FAIL_SUCCESS(EXPRESSION)

#define SILENT_EXPECT(EXPRESSION)                                            \
  if(!(EXPRESSION)) {                                                       \
    (*self) << fetch::commandline::VT100::Return                            \
            << fetch::commandline::VT100::Right(70);                        \
    (*self) << " [ " << fetch::commandline::VT100::Bold;                    \
    (*self) << fetch::commandline::VT100::GetColor("red", "default");       \
    (*self) << "FAIL" << fetch::commandline::VT100::DefaultAttributes()     \
            << " ]\n\n\n";                                                  \
    (*self) << "Expect failed " << __FILE__ << " on line " << __LINE__      \
            << ": \n\n";                                                    \
    (*self) << "    " << #EXPRESSION << "\n\nwhich expands to:\n\n    ";    \
    (*self) << (fetch::unittest::ExpressionStart() * EXPRESSION) << "\n\n\n\n"; \
    exit(-1);                                                               \
 }
  
#define DETAILED_EXPECT(EXPRESSION)                                     \
  fetch::unittest::details::NewNestedProgram([=](                             \
      fetch::unittest::ProgramInserter::sub_function_type sub) {              \
    if (!(EXPRESSION)) {                                                      \
      (*self) << "Expect failed " << __FILE__ << " on line " << __LINE__      \
              << ": \n\n";                                                    \
      (*self) << "    " << #EXPRESSION << "\n\nwhich expands to:\n\n    ";    \
      (*self) << (fetch::unittest::ExpressionStart() * EXPRESSION) << "\n\n"; \
      sub(self);                                                              \
      exit(-1);                                                               \
    }                                                                         \
  });                                                                         \
  fetch::unittest::details::last_inserter() = [=](                      \
      fetch::unittest::TestContext::self_shared_type self)
};
};
#endif
