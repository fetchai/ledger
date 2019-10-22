# C++ code style guidelines

We use C++14, but currently consider C++17 too immature for usage.

This documents contains guidelines regarding:

* File structure
* Comments and documentation
* Tests
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
	1. Related header.
	2. `C` library.
	3. `C++` library.
	4. Other libraries.
	5. Fetch library.

* Classes in this order
	1. `public`.
	2. `protected`.
	3. `private`.


## Comments and documentation

* Provide API documentation with your code.
* Include a comment summary at the top of all files.

``` c++
/* Approximation of the exponential function.
 * @x is the argument that is used in the exponent 2^(x / M_LN2).
 *
 * We exploit the IEEE-754 standard to efficiently create a linear
 * interpolation between points of the form 2^n where n is an integer.
 *
 * You can find the full details in
 * [1] "A fast, compact approximation of the exponential function",
 * Nicol N. Schraudolph, 1999 Neural computation 11, 853-862.
 *
 * @return the exponential of x.
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


## Tests

* Write tests and run them
* Do not assume, do assert


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
namespace fetch
{
  namespace details
  {
    struct FetchImpl
    {
      /* ... */
    };

    void HelperFunction(FetchImpl &s)
    {
      /* ... */
    }
    };  // namespace details

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
  };
};
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
  1. Inline initialisation.
  2. Constructor initialiser list.
  3. Constructor body.

* Mark all constructors and assignment operators explicitly, even those which are implicit, to ensure there is no confusion about the intended behaviour of the class.
* Mark unneeded default implementations as deleted. C++ infers whether to create implicit constructors based on whether there are user defined ones. This means that when someone declares a constructor, C++ may remove the default constructor.

``` c++ class Transaction
{
public:
  Transaction()                    = default;
  Transaction(Transaction const &) = default;
  Transaction(Transaction &&)      = deleted;

  ~Transaction() = default;

  Transaction operator(Transaction const &) = default;
  Transaction operator(Transaction &&)      = deleted;
}
```

* Mark a class `final` if it is not meant to be subclassed. The same counts for final implementations of virtual functions.

``` c++
class Foo
{
public:
  virtual ~Foo()                        = default;
  virtual void Greet()                  = 0;
  virtual void Say(std::string const &) = 0;
};

class Bar : public Foo
{
public:
  void Greet() override final
  {
    // ...
  }

  void Say(std::string const &what) override {
      // ...
  };
};

class Baz final : public Bar
{
public:
  void Say(std::string const &what) override
  {
    // ...
  }
};
```

Inline initialisation of member variables is preferred at all times, as uninitialised memory can have secondary effects, making it almost impossible to debug a problem.

When it is not possible to use inline initialisation, member variables should be initialised using the constructor initialisation list where they must be listed in the order they appear.

Last resort is initialisation inside the constructors body. In most cases, a combination of the above will be used. For example:

``` c++
class MyClass
{
public:
  MyClass() = default;

  explicit MyClass(std::size_t n)
    : size_(n)
  {
    pointer_ = new int[n];
    for (std::size_t i = 0; i < n; ++i)
    {
      pointer_[i] = 0;
    }
  }
  // ...

private:
  std::size_t size_    = 0;
  int *       pointer_ = nullptr;
};
```

Here, inline initialisation ensures that the default constructor does not produce uninitialised memory. In the second constructor, we manually initialise the `pointer_` but `size_` can be set using the initialisation list.


## Error handling

* Always check error codes and return codes.
* Use exceptions *only* for exceptional circumstances.
* Handle normal errors gracefully.

* Do not rely on exceptions to avoid taking responsibility.
* Do not throw exceptions in destructors as this can crash the program.


## Memory handling

* Use references where the value cannot be null. Use pointers where it can.
* Make sure no uninitialised memory is accessed.
* Always prefer passing by reference or `const` reference where possible.
* Prefer smart over raw pointers.
* Use `make_shared` and `make_unique` to wrap objects in smart pointers.
* Stick to RAII principle and automatic memory management - destructors are ideally empty.

### nullptr

Use the `nullptr` keyword which designates an `rvalue` constant serving as a universal null pointer literal. This replaces the buggy and weakly-typed literal `0` and the infamous `NULL` macro.

`nullptr` is also type safe.

### Handling dynamically allocated memory resources

Use smart pointers from the `std::` namespace dedicated to memory management, such as the most frequently used:

* `std::shared_ptr<...>`.
* `std::unique_ptr<...>`.

Please note, the *default* delete policy for these types uses the **default** C++ delete operator for **single** objects. However, the API of these types offers custom deleter and allocator functions for managing non-default allocation/deallocation policies such as needed when managing **array** object types.

Below are some types which guarantee to manage **array** object types (continuous block of memory) by default, and other types which do memory management as a side effect of their primary business logic.

* `std::vector<...>` guarantees to manage a continuous block of memory (array) by definition.
* `std::string<...>` guarantees to manage a continuous block of memory (array) by definition.
* `SharedArray<...>` and `Array<...>` classes (from `fetch::memory` namespace) that guarantee to manage a continuous block of memory (array) by definition.
* Classes from `fetch::byte_array` namespace, such as `BasicByteArray`.

### Transfer between ownership models

It is safe to transfer ownership from `std::unique_ptr` to `std::shared_ptr`.

However, do **NOT** transfer ownership the other way around, as that will result in memory access exceptions; a non-owning `std::weak_ptr` should be used instead of `std::unique_ptr` (see [reference documentation](https://en.cppreference.com/w/cpp/memory/weak_ptr)).

Example of using smart pointer with **shared** ownership:

``` c++
#include <iostream>
#include <memory>

// Parent scope block
{
  std::shared_ptr<int> a0;

  // Nested scope block
  {
    std::shared_ptr<int> a1 = std::make_shared<int>(5);  // refcount=1
    a0                      = a1;  // Data in `a1` becomes shared (refcount=2)

    // BOTH instances are valid, controlling the same `int` instance:
    std::cout << "a0 = " << *a0 << ", a1 = " << *a1 << std::endl;

    throw std::exception();

    // The exception will cause stack unwinding, during which the destructor
    // of `a1` is called automatically, decrementing the refcount of shared
    // object (to 1).
    // At this point, the `a0` shared object is controlling lifecycle
    // of the `int` instance created above in this scope.
  }

  // We are still in stack unwinding process here (due to earlier exception),
  // during which automatically called destructor of `a0` instance decrements
  // refcount of shared object (to 0), which finally results in destruction of
  // the `int` instance allocated & initialised in the nested scope above.
}
```

Example of using smart pointer with **exclusive** ownership:

``` c++
#include <memory>

// Parent Scope block
{
  std::unique_ptr<int> a0;

  // Nested scope block
  {
    std::unique_ptr<int> a1 = std::make_unique<int>(5);
    a0                      = a1;  // Transfer of OWNERSHIP from `a1` to `a0`
    // `a1` does NOT control lifecycle of the `int` instance anymore.

    // This will result in throwing an exception since `a1` is now `nullptr`
    std::cout << "a1=" << *a1 << std::endl;

    // The automatically called destructor of `a1` does NOTHING since
    // `a1` does NOT control any object.
  }

  // Automatically called destructor of `a0` destroys the `int` instance
  // allocated and initialised in the nested scope above.
}
```


## Resource handling

It is important to manage resources whose lifecycle needs strict control, such as:

* Thread synchronisation constructs (e.g. mutex).
* Filesystem objects.
* Network connections.
* Database sessions.

This is especially critical in environments where exceptions can occur.

Use a scope based smart pointer concept to manage resource lifecycle. The `Smart Pointer` concept exploits the C++ native constructor-destructor (unbreakable) bond for the class/struct instance created on the stack using <a href="https://en.cppreference.com/w/cpp/language/direct_initialization" target=_blank>direct initialisation</a> where its lifecycle is strictly controlled by the encapsulating scope inside of the created instance.

The most important aspect of this strong bond is that the destructor is *automatically* called when the code flow is exiting the encapsulating scope. This includes exits caused by an exception being thrown.

Below are a few examples of how resources should be managed in the code.

### Exception enabled environment and its impact on resource handling

The *most important rule* is that the destructor will **NEVER** throw an exception in any circumstance.

The reason for this is that a destructor for a *directly* initialised object (the lifecycle of which is controlled by a scope) is called automatically when any exception is thrown within that scope. This means that if the destructor throws yet another exception, it forces the C++ runtime to call `std::terminate` of the process (as per the C++ standard definition), since the C++ runtime is already in the <a href="http://en.cppreference.com/w/cpp/language/throw" target=_blank>stack unwinding</a> process caused by the first exception.

See more about this scenario at <a href="http://en.cppreference.com/w/cpp/language/destructor#Exception" target=_blank>cppreference.com</a> and at <a href="http://www.codingstandard.com/rule/15-2-1-do-not-throw-an-exception-from-a-destructor/" target=_blank>codingstandard.com</a>.

### Handling `THREAD` synchronisation resources

Use locking guards provided in C++ `std` namespace, such as:

* `std::unique_lock<...>`.
* `std::lock_guard<...>`.

The Fetch Ledger codebase offers a couple implementations of mutex in the `fetch::mutex` namespace which conform to a `std` locking API contract. They can be used with the above-mentioned locking guard constructs from `std` namespace. Please prefer them in favour of plain `std::mutex`:

* `fetch::mutex::DebugMutex`
* `fetch::mutex::ProductionMutex`

### The `std::unique_lock<...>`

The `std::unique_lock` is a superset of the `std::lock_guard` in terms of the API and its capabilities.

It offers more flexibility and allows the following:

* The constructor of the class can take a lockable object (e.g. mutex) in any state (locked or unlocked), and thus `adopting` its pre-existing lock state.
* It can lock & unlock lockable objects when desired (as many times as necessary).
* It can release control of the lockable object it controls.
* Its destructor unlocks the lockable object if it is in locked state.


``` c++
#include <condition_variable>
#include <mutex>
#include <thread>

// A condition used in cooperation with `unique_lock<...>`
// to retain state across lock and signal change of the state
std::condition_variable condition;

// Variable keeping the state
bool a_state = 0;

// Global mutex variable
std::mutex(mtx);

auto increment_fnc()->void
{
  {
    // this constructor locks mutex automatically
    std::unique_lock<mutex> lock(mtx);

    // Updating value of state keeping variable (in thread safe way - see comment bellow)
    ++a_state;

    // It is *not* necessary to explicitly call `lock.unlock()` since
    // it will be called implicitly by destructor of the `lock` object.
  }

  // Signal on condition to notify *ALL* listeners waiting for the signal.
  // Mind to note here that this is **independent** from specific
  // synchronisation object (like mutex), reason being it can be signalled
  // from anywhere (e.g. outside of mutex locked scope).
  condition.notify_all();
}

auto decrement_fnc()->void
{
  std::unique_lock<mutex> lock(mtx);

  // Waiting for condition to signal.
  // An std::function (e.g. lambda) can be passed to prevent spurious
  // wake ups, for example condition.wait(lock, [](){/* ... */ return A_CONDITION;})

  // The `wait(lock)` implicitly UNlock the mutex **BEFORE** falling
  // to wait state (essentially relocating thread to wait queue on OS level).
  condition.wait(lock, [orig_state = a_state, &state = a_state]() {
    // Here we are trying to figure whether the state has changed since we started
    // to monitor/listen to the signalling.
    if (orig_state < state)
    {
      --state;  // Modifying state
      return true;
    }
    return false;
  });
  // The `wait(lock)` implicitly reacquire back (locks) the mutex after it handled
  // signalled condition.

  // It is *not* necessary to explicitly call `lock.unlock()` since
  // it will be called implicitly by destructor of the `lock` object.
}

int main(int argc, char *argv[])
{
  std::thread t1([]() { increment_fnc(); });

  std::thread t2([]() { decrement_fnc(); });

  t1.join();
  t2.join();
}
```

### The `std::lock_guard<...>`

The `std::lock_guard<T>` offers the most trivial API possible - constructor and destructor.

It locks the synchronisation object of `T` type **immediately** at construction time, and releases the lock in the destructor.

The constructor of this class is able to take a lockable object (e.g. mutex) in any state (locked or unlocked) and thus 'adopting' its pre-existing lock state. For example:

``` c++
#include <mutex>

std::mutex(mtx);

{
  std::unique_lock<mutex> guard(mtx);

  // Some code desired to be executed thread safe manner.

  // Automatically called destructor of the `guard` instance
  // will implicitly unlock the mutex.
}
```


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
* TODOs should refer to the author in the style `TODO(`HUT`) :`.
* Avoid leaving `TODOs` in code.
* Use templates judiciously and defined to library code where only a small number of developers need to understand them.
