# C++ code style guidelines

We use C++14, but currently consider C++17 too immature for usage.

This documents contains guidelines regarding:

* File structure
* Comments and documentation
* Naming
* Formatting
* Typing
* Namespaces
* Structs
* Classes
* Error handling
* Memory handling
* Resource handling
* Other


## File structure

* Header files: `.hpp`
* Source files: `.cpp`
* Use `#pragma once` in headers.
* Split class definition across a header and source file by default.
* Include header definitions in the same file.

* Order of includes
	1. Internal library headers
	2. Other Fetch library headers
	3. Vendor library headers
	4. System/standard library headers

* Class access specifiers in this order
	1. `public`
	2. `protected`
	3. `private`


## Comments and documentation

* Provide API documentation with your code.
* Include a comment summary at the top of all files.

``` c++
/**
 * Approximation of the exponential function.
 *
 * We exploit the IEEE-754 standard to efficiently create a linear
 * interpolation between points of the form 2^n where n is an integer.
 *
 * You can find the full details in
 * [1] "A fast, compact approximation of the exponential function",
 * Nicol N. Schraudolph, 1999 Neural computation 11, 853-862.
 *
 * @param x the argument that is used in the exponent 2^(x / M_LN2).
 * @returns The exponential of x
 */
double Exp(double const &x)
{
  // We use the constants explained in [1] and additional make e the
  // base rather than using 2.
  static double const a = double(1ull << 20) / M_LN2;
  static double const b = ((1ull << (10)) - 1) * double(1ull << 20) - 60801;
  double in = x * a + b;

  // Move the computed bits into the exponent with spilling into the
  // Mantissa. The spilling creates the linear interpolation, while
  // the exponent creates exact results whenever the argument is an integer.
  union
  {
    double   d;
    uint32_t i[2];
  } conv;

  conv.i[1] = static_cast<uint32_t>(in);
  conv.i[0] = 0;

  return conv.d;
}

```

* Comment the non-obvious, not the obvious.

``` c++
// Turns out sorting this list before processing it is faster due to branch
// prediction, so don't remove this without checking the profile:
std::sort(users.begin(), users.end(), UserSortPredicate);
for (User const &user : users)
{
  if (user.ready)
  {
    // Do something...
  }
}
```

* If you do not understand some code, contact the original author. If they have left the company ask the most competent person. After working out what a piece of a non-obvious code is doing, remember to document it.


## Naming

* Use meaningful names, even in tests.
* For struct and class names, use `PascalCase`.
* Private and protected class member variables end with an `_`.
* Variable names are `snake_case_` with trailing underscore, except in structs where it is `snake_case` without underscore.
* For templates, use all uppercase with underscores to separate words. Long and explicit names are preferred to short and obscure names, e.g. `template <typename ARRAY_TYPE>` is preferable to `template <typename A>);`. Short names are acceptable where the type is obvious and reused many times in complex equations.
* For enums use `UPPER_CASE`.
* For macros use `UPPER_CASE`.


## Formatting

* Make use of the `apply_style` script provided for formatting.
* Lines should be no longer than 100 characters.
* Use 2 spaces to indent, not tabs.
* Curly brackets `{...}` should be on their own line.
* There should be a space after conditionals, i.e. `if (test)`
* No spaces inside `(parenthesis)` or `<template>` unless it improves readability.
* No spaces around arrow: `this->fnc();`.
* No trailing whitespace.
* Align attributes by group in classes, and initialisers.
* Variables should be initialised with `=` if primitive, or braced initialisation with parentheses if required.
* Initialiser lists should be initialised with braces by default.
* Use SQL style initialiser for lists and inheritance lists.

```c++
Foo::Foo()
  : attribute1_{value1}
  , attribute2_{value2}
{}
```


## Typing

* Use STL types wherever possible.
* Prefer `(u)int(16/32/64)_t` and `std::size_t`.
* Use `static_cast`, `reinterpret_cast`, `dynamic_cast`, etc., rather than C-style casts.
* Avoid `auto` except when the return type is clear.
* Avoid use of RTTI.

* `using` is preferable to `typedef` for type alias definition.
* Make `using` private unless you intend to expose it.
* `namespace {.<template T>` only allowed if `typedef`/`using` indicates the type.

* If a value is `const`, declare it so.
* Use `int const &i` over `const int &i`.

* Prefer `enum class` over regular `enum`.


## Namespaces

* Do not do `using namespace foo` or `namespace baz = ::foo::bar::baz` in `.hpp` files.
* Do not do `using namespace foo` in the top `fetch` namespace, as it pollutes all of it. Instead, do the following:

```c++
namespace fetch {
namespace details {

struct FetchImpl
{
  /* ... */
};

void HelperFunction(FetchImpl &s)
{
  /* ... */
}

}  // namespace details

class MyFetchClass
{
public:
  void DoSomeThing()
  {
    // This improves readability
    using namespace details;

    FetchImpl f;
    HelperFunction(f);
  }
  // ...
};

}  // namespace fetch
```


## Structs

Use structs for Plain Old Data (POD) and, in some cases, template meta-programming. Use `classes` for everything else.

``` c++
struct Person
{
  std::string name;
  int         age;
  // ...
};
```


## Classes

* Initialise class members in the following order of preference:
  1. Inline initialisation. This ensures, for instance, that the default constructor does not produce uninitialised memory.
  2. Constructor initialiser list. Member variables must be listed in the order they appear.
  3. Constructor body.

In most cases, a combination of the above will be used. For example:

``` c++
class MyClass
{
public:
  // Inline initialisation
  MyClass() = default;

  // Initialisation list for one attribute, constructor body for another
  explicit MyClass(std::size_t n)
    : size_(n)
  {
    doubled_ = n * 2;
  }
  // ...

private:
  std::size_t size_    = 0;
  std::size_t doubled_ = 0;
};
```

* Mark all constructors and assignment operators explicitly, even those which are implicit, to ensure there is no confusion about the intended behaviour of the class.
* Mark unneeded default implementations as deleted. C++ infers whether to create implicit constructors based on whether there are user defined ones. This means that when someone declares a constructor, C++ may remove the default constructor.

``` c++ class Transaction
{
public:
  Transaction()                    = default;
  Transaction(Transaction const &) = default;
  Transaction(Transaction &&)      = deleted;
  ~Transaction()                   = default;

  Transaction operator(Transaction const &) = default;
  Transaction operator(Transaction &&)      = deleted;
}
```

* Mark a class `final` if it is not meant to be subclassed. The same counts for final implementations of virtual functions.


## Error handling

* Always check error codes and return codes.
* Use exceptions *only* for exceptional circumstances.
* Handle normal errors gracefully.

* Do not rely on exceptions to avoid taking responsibility.
* Do not throw exceptions in destructors, in order to prevent calls to `std::terminate` during stack unwinding (as per the [C++ standard definition](http://en.cppreference.com/w/cpp/language/destructor#Exceptions)).


## Memory handling

* Use references where the value cannot be null. Use pointers where it can.
* Make sure no uninitialised memory is accessed or created.
* Always prefer passing by reference or `const` reference where possible.
* Prefer smart pointers (`std::shared_ptr`, `std::unique_ptr`, etc) over raw pointers.
* Use `make_shared` and `make_unique` to wrap objects in smart pointers.
* Prefer the type-safe `nullptr` over the weakly-typed literal `0` and the `NULL` macro.
* Stick to RAII principle and automatic memory management - destructors are ideally empty.
* When working with array object types, prefer existing structures that already implement memory management:
  * From the standard library, such as `std::vector` and `std::string`.
  * `SharedArray` and `Array` classes from `fetch::memory` namespace.
  * Classes from `fetch::byte_array` namespace, such as `BasicByteArray`.


## Resource handling

* Use the locking guard `FETCH_LOCK` whenever possible, and only those provided in C++ `std` namespace (such as `std::unique_lock` and `std::lock_guard`) when necessary.
* Prefer implementations of mutex in the `fetch::mutex` namespace (which conform to a `std` locking API contract) over a plain `std::mutex`. These are `fetch::mutex::DebugMutex` and `fetch::mutex::ProductionMutex`.


## Other

* Don't use `#define`. If you ignore this guidance, choose a `FETCH_STARTING_REALLY_LONG_NAME_THAT_CANNOT_POSSIBLY_CLASH_WITH_ANYTHING_ELSE` and undefine it as soon as you are done with it.
* Use the following keywords when appropriate:
	1. `explicit`.
	2. `override`.
	3. `virtual`.
	4. `final`.
	5. `noexcept`.
* Pre increment: `++i` as it is more performant for non primitive types.
* Lambda captures must be explicit reference or copy.
* TODOs should refer to the author in the style `TODO(HUT) :`.
* Avoid leaving `TODOs` in code.
* Use templates judiciously and defined to library code where only a small number of developers need to understand them.
