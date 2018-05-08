#<cldoc:Developers Guide::C++ Style Guide>

A guide that describes the style of our code.
 
*This is an initial rewrite of what's on the wiki - to be revised!.*

# Summary
To make it nice and simple to get started we here make a brief summary
of the style guide. Some of the choices in this guide may be "arbitrary"
but should be followed nonetheless as the purpose of a style guide is to
make the code look and feel like it was written by a single skilled
programmer. With that in mind, here is a brief summary of the contents
of this guide:

1. First and formost, use STL types whenever you can. If not appropriate,
**always** check error codes and return codes. Great amounts of example
code omit error checking to keep things simple. More security and
stability errors occur due to piss-poor error checking than many other
things. Do **not** rely on exceptions to avoid having to take any
responsibility.

2. Do not assume, do `assert`. Adding `assert` whenever you can while
make your debugging code a bit slower, but will have no impact on your
release code.

3. Do not assume, write tests. Your code may work when you commit it to
the repository, but when you later on decide to integrate it into other
parts of the system or refactor, the changes you make may break your
code. If you have a proper test suite in place, this will not go
unnoticed.

4. Always be const correct. If it is const, declare it as such. E.g., if
you have a member function that does not modify member variables,
returns a constant value and takes one constant parameter, *get it
right*: 

5. Comment the non-obvious, not the obvious and provide API
documentation together with your code such that it is easy for other to
understand how to use your codes. 

6. Prefer `shared_ptr` and `unique_ptr` over raw pointers. Use
references where the value cannot be null, use pointers where it
can. Always prefer passing by const reference or reference where
possible.

7. Don't use `#define`. And if you decide to do this anyway, then chose a
`FETCH_STARTING_REALLY_LONG_NAME_THAT_CANNOT_POSSIBLY_CLASH_WITH_ANYTHING_ELSE`
and undefine it as soon as you are one with it.

8. Use `static_cast`, `reinterpret_cast`, `dynamic_cast` etc. rather
than C-style casts, as this is explicit and easy to understand.

9. Any warning is an error. A code committed to the repository should be
pruned for warnings as these are errors occuring at runtime and actually
not warnings. Compile with `-Wall -Werror`.

10. Lines should be no longer than 80 characters. `apply_style` can fix
this for you.

11. Use spaces, not tabs.

12. Curly brackets `{}` should be on their own line.

13. There should be a space after conditionals: `if (test)`

14. Use meaningful names.

15. Use C++11 features, but not C++14 or C++17.

16. Structs are for passive data structures, classes for active.

17. Explicitly declare default constructors and operators.

18. Initialize class members with highest preference for inline
initialization, second highest in the constructor initializer list and
lowest in the contstructor body. Uninitialized memory can cause serious
bugs and one should actively try to avoid creating it.

19. Use `nullptr`, not 0 or `NULL`.

20. Try to include headers for your definitions in the same file rather than relying
on it being defined in another include - this prevents code breaking when that header
is changed.

# Detailed guide

Below we give some more detailed examples and explanations for the above rules.

## Formatting
Use spaces, not tabs. Make indenting

To assist you formatting your code, you can make use of the
`apply_style` script provided.


## Naming
Use meaningful names.

For stucts, member variables are named with all lower case using
underscore to separate words, or mixed case using upper case to separate
words. For instance, "first_name" or "firstName" are valid variable
member names for structs.

For classes, we follow the convention 

## Commenting and documenting
Code can and should be documented, both inline and on an API. An
important part of documenting is chosing good names but also to make
comments where appropriate. All comments must use a professional
language. The source code is no place for jokes or expletives in
comments. Don't

```
  // Computes exponential using magic
  double Exp(double const &x) {
    // No idea where these magic constants come from
    static double const a = double(1ull << 20) / M_LN2;
    static double const b = ((1ull << (10)) - 1) * double(1ull << 20); - 60801;
    double in = x * a + b;

    union { // How the f$#k does this work?
      double d;
      uint32_t i[2];
    } conv;

    conv.i[1] = static_cast<uint32_t>(in);
    conv.i[0] = 0;

    return conv.d;
  }
```

Do

```
  /* Approximation of the exponential function.
   * @x is the argument that is used in the exponent 2^(x / M_LN2).
   *
   * We exploit the IEEE-754 standard to effeciently create a linear
   * interpolation between points of the form 2^n where n is an integer.
   *
   * You can find the full details in
   * [1] "A fast, compact appraoximation of the exponential function",
   * Nicol N. Schraudolph, 1999 Neural computation 11, 853-862.
   *
   * @return the exponential of x.
   */
  double Exp(double const &x) {
    // We use the constants explained in [1] and additional make e the
    // base rather than using 2.
    static double const a = double(1ull << 20) / M_LN2;
    static double const b = ((1ull << (10)) - 1) * double(1ull << 20); - 60801;
    double in = x * a + b;

    // Move the computed bits into the exponent with spilling into the
    // Mantissa. The spilling creates the linear interpolation, while
    // the exponent creates exact results whenever the argument is an integer.     
    union { 
      double d;
      uint32_t i[2];
    } conv;

    conv.i[1] = static_cast<uint32_t>(in);
    conv.i[0] = 0;

    return conv.d;
  }
```

If you do not understand a code, contact the original author. If he or
she has left the company ask the most competent person. After working
out a piece of a non-obvious code is doing, remember to document it.

Finally, comment the non-obvious:
```
// Turns out sorting this list before processing it is faster due to branch prediction, so don't
// remove this without checking the profile:
std::sort(users.begin(), users.end(), UserSortPredicate());
for (const User& user: users)
{
    if (true == user.ready)
    {
        // Do something...
        
    }   // if (user ready)
}   // iterate (users)
```

## Use of C++11
We use C++11, but currently consider C++17 to immature for usage.


## Namespaces and using namespaces
Never pollute the any namespace using `using namespace std;`. Using
namespaces can be useful in limited scopes, such as within a
funciton. Don't

```
namespace fetch {
using namespace std; // Pollutes all of the fetch namespace and causes
                     // problems for every one.

class MyFetchClass {
public:
  void Greet() {
    cout << "Hello" << endl;
  }
  // ...
};

};
```

Do


```
namespace fetch {
namespace details {
  struct FecthImpl { /* ... */ };
  
  void HelperFunction(FecthImpl & s) {
     /* ... */
  }  
};

class MyFetchClass {
public:
  void DoSomeThing() {
    // This improves readability
    using namespace details; 

    FetchImpl f;
    HelperFunction(f);
  }
  // ...
};

};
```


## Structs
We use structs for passive data and, in some cases, template meta
programming. Everything else is done by `classes`. An example of a
passive data structure could be a persons record

```
struct Person {
  std::string name;
  int age;
  // ...
};
```

## Classes

Always mark all constructors and assignment operators explicitly, even
those which are the implicitly. For instance,

```
class Transaction {
public:
  Transaction() = default;
  Transaction(Transaction const&) = default;
  Transaction(Transaction &&) = deleted;

  ~Transaction() = default;

  Transaction operator(Transaction const&) = default;
  Transaction operator(Transaction &&) = deleted;
}
```
In this way there will be no confusion about the intended behaviour of
the class. Likewise, for those default implementations that are not
needed, mark them as deleted.

If a class is not meant to be subclassed, mark it as `final`. The same
counts for final implimentations of virtual functions. For example

```
class Foo {
public:
  virtual ~Foo() { }
  virtual void Greet() = 0;
  virtual void Say(std::string const&) = 0;   
};

class Bar : public Foo {
public:
  void Greet() override final {
    // ...
  }
  
  void Say(std::string const& what) override {
    // .. 
  };     
};

class Baz final : public Bar {
public:
  void Say(std::string const& what) override {
    // ...
  } 
}; 

```

C++11 allows for inline initialization of your member variables. This
should be preferred at all times as uninitialized memory can have
secondary effects that makes almost impossible to debug a problem. When
it is not possible to use inline initialization, the member variables
should be initialized using the constructors initialization list where
they must be listed in the order they appear. Last resort, is
initialization inside the constructors body. This should only be used
when the the two previous are not applicable. In most cases, a
combination of the above will be used. As an example:

```
class MyClass {
public:
  MyClass() = default;
  
  MyClass(std::size_t n)
  : size_(n) {
    pointer_ = new int[n];
    for(std::size_t i=0; i< n; ++i)
      pointer_[i] = 0;    
  }
  // ...
private:
  std::size_t size_ = 0;
  int* pointer_ = nullptr;
};
```
In this code, the inline initialization ensures that the default
constructor does not produce uninitialized memory. In the second
constructor, we manually need to initialize the `pointer_` but `size_`
can be set using the initialization list.

## Usage of `nullptr`
The new C++11 `nullptr` keyword designates an rvalue constant that
serves as a universal null pointer literal replacing the buggy and
weakly-typed literal 0 and the infamous `NULL` macro. `nullptr` thus
puts an end to more than 30 years of embarrassment, ambiguity, and bugs.  
Furhter, the new nullptr is type safe.


## Resource handling
This section describes how to manage resources which lifecycle needs to be strictly controlled, such as:
  * thread synchronisation constructs (e.g. mutex),
  * memory,
  * filesystem objects,
  * network connections,
  * database sessions,
  * etc. ...

This is especially critical in environment where **exceptions** can occur (see dedicated section bellow). 

It is suggested to use scope based smart pointer concept to manage lifecycle for resources. The `Smart Pointer` concept exploits C++ native constructor-destructor (unbreakable) bond for the class/struct instance created on the stack (using [direct initialisation](http://en.cppreference.com/w/cpp/language/direct_initialization)), where its lifecycle is strictly controlled by encapsulating scope inside of which the instance has been created.

The most important aspect of this strong bond is that the destructor is *automatically* called when the code flow is exiting the encapsulating scope, what **includes** exit caused by exception being thrown. 
Bellow are a few examples of how resources should be managed in the code:


### Exception enabled environment and it's impact on the resource handling

The most important rule is that destructor **shall NOT** throw an exception at any circumstance.
   * The reason being that a destructor for a *directly* initialised object (lifecycle of which is controlled by a scope) will be called **automatically** when any exception is thrown in that scope.
   * Where implication is that **IF** such destructor would throw yet another exception, it would force C++ runtime to call **std::terminate** of the process(by definition of C++ standard), since the C++ runtime is already in [stack unwinding](http://en.cppreference.com/w/cpp/language/throw) process caused by the first exception.

Please see more about this scenario at [cppreference.com](http://en.cppreference.com/w/cpp/language/destructor#Exceptions) and at [codingstandard.com](http://www.codingstandard.com/rule/15-2-1-do-not-throw-an-exception-from-a-destructor/).
### Handling THREAD synchronisation resources
Please use locking guards provided in C++ `std` namespace, such as:
   * `std::unique_lock<...>`, or
   * `std::lock_guard<...>`.

The `std::unique_lock` is **superset** of the `std::lock_guard` in terms of API and capabilities.
It offers more flexibility, which allows to do following things:
   * constructor of this class is able take on board lockable object (e.g. mutex) in any state (locked or unlocked), so `adopting` its pre-existing lock state,
   * it is able to lock & unlock lockable object when desired (as many times as necessary),
   * release control of the lockable object it controls,
   * it's destructor unlocks the lockable object **if** it is in locked state.

> See the example bellow:

```cpp
#include <condition_variable>
#include <mutex>
#include <thread>

//* A condition used in cooperation with `unique_lock<...>`
//* to retain state across lock and signal change of the state
std::condition_variable condition;

//* Variable keeping the state  
bool a_state = 0;

//* Global mutex variable
std::mutex(mtx);

auto increment_fnc() -> void
{
  //* this constructor locks mutex automatically
  std::unique_lock<mutex> lock(mtx);
  
  //* Updating value of state keeping variable (in thread safe way - see comment bellow)
  ++a_state;

  //* Signal on condition to notify *ALL* listeners waiting for the signal.
  //* Mind to note here that this is independent from specific
  //* synchronisation object (like mutex), reason being it can be signalled
  //* from anywhere.
  //* However, in general, it is good to call it from scope where
  //* underlying thread synchronisation object is locked since it is usually
  //* the case that this signalling is done when state keeping variable
  //* changes value, what must be done in thread safe way.
  condition.notify_all();
  
  //* It is *not* necessary to explicitly call `lock.unlock()` since
  //* it will be called implicitly by destructor of the `lock` object. 
}

auto decrement_fnc() -> void
{
  std::unique_lock<mutex> lock(mtx);

  //* Waiting for condition to signal.
  //* An std::function (e.g. lambda) can be passed to prevent spurious
  //* wake ups, for example condition.wait(lock, [](){/* ... */ return A_CONDITION;})
  
  //* The `wait(lock)` implicitly UNlock the mutex **BEFORE** falling
  //* to wait state (essentially relocating thread to wait queue on OS level).
  condition.wait(lock, [orig_state = a_state, &state = a_state](){
      //* Here we are trying to figure whether the state has changed since we started
      //* to monitor/listen to the signalling.
      if (orig_state < state) {
        --state; //* Modifying state
        return true;
      }
      return false;
    });
  //* The `wait(lock)` implicitly reacquire back (locks) the mutex after it handled
  //* signalled condition.

  //* It is *not* necessary to explicitly call `lock.unlock()` since
  //* it will be called implicitly by destructor of the `lock` object. 
}

int main(int argc, char* argv[]) {

    std::thread t1([]() {
        increment_fnc();
      });

    std::thread t2([]() {
        decrement_fnc();
      });

    t1.join();
    t2.join();
}
```


The `std::lock_guard<T>` offers the most trivial API possible - constructor and destructor.
It locks the synchronisation object of `T` type **immediately** during construction time, and releases the lock in destructor.
Constructor of this class is able take on board lockable object (e.g. mutex) in any state (locked or unlocked), so `adopting` its pre-existing lock state.

> See the simplified example bellow:

```cpp
#include<mutex>
std::mutex(mtx);

{
  std::unique_lock<mutex> guard(mtx);

  //* Some code desired to be executed thread safe manner.

  //* Automatically called destructor of the `guard` instance
  //* will implicitly unlock the mutex.
}
```

### Handling dynamically allocated MEMORY resources
Please use smart pointers from `std::` namespace dedicated to memory management. The most frequently used smart pointers are `std::shared_ptr<...>`, `std::unique_ptr<...>`, etc. ...

Here we can also mention container classes, most notably the `std::vector<...>`, which also manage memory for us.

* It is **discouraged** to store or pass around **raw** pointers in the code, unless there is really strong reason to do so (e.g. cooperating with C code, or third party library, which API requires to pass raw pointers, etc. ...), what should be judged during the review process.

* Please be aware of one twist in C++ language affecting lifecycle management of dynamic memory allocation: **single** object vs. **array** object allocation & deallocation. There are 2 versions of `new` & `delete` operators:
   * one for **single** object (`ptr = new T (...)` & `delete ptr`), and 
   * another one for **array** object (`ptr = new [size] (...)` & `delete [] ptr`)
   , where new & delete operator version are tangled together for each case:
      - If something has been constructed using **single** object new operator version, then it **must** be destructed using single object version of delete operator,
      - and vice versa, if something has been constructed using **array** object new operator, the it **must** be destructed using delete operator for array object.

* Transfer between ownership models.
   1. It is safe to transfer ownership from `std::unique_ptr` to `std::shared_ptr`.
   1. Do **NOT** transfer ownership other way around, from `std::shared_ptr` to `std::unique_ptr`.<br/>
      First of all - it is really highly questionable to justify reason for transfer between ownership models in this direction!<br/>
      Second of all - if you mind look up at `First of all - ...` again, and then, and only then, if you still think it is really really necessary to do this for some obscure reason, then keep in mind that:
      * this is **non**-trivial operation,
      * that `shared_ptr` type has **NOT** been designed with mind to allow this,
      * if you are still convinced to proceed, then [here](https://stackoverflow.com/questions/15337461/move-ownership-from-stdshared-ptr-to-stdunique-ptr) is some guide to do such a thing.

> Example of how to use smart pointer with **shared** ownership:

```cpp
#include <memory>
#include <iostream>

//* Parent Scope block
{
  std::shared_ptr<int> a0;
  
  //* Nested scope block
  {
    std::shared_ptr<int> a1 = std::make_shared<int>(5); //* refcount=1
    a0 = a1; //* The `a1` instance become shared at this point (refcount=2)
    
    //* BOTH instances are valid, controlling the same `int` instance:
    std::cout << "a0 = " << *a0 << ", a1 = " << *a1 << std::endl;
    
    throw std::exception();
    
    //* The exception will cause stack unwinding, during which 
    //* is the destructor of `a1` instance called automatically, 
    //* what decrements refcount of shared object (to 1).
    //* At this point, the `a0` shared object is controlling lifecycle
    //* of the `int` instance created above in this scope.
  }
  
  //* We are still in stack unwinding process here (due to earlier exception),
  //* during which automatically called destructor of `a0` instance  decrements
  //* refcount of shared object (to 0), what finally results to destruction of
  //* the `int` instance allocated & initialised in the nested scope above.
}
```

> Example of how to use smart pointer with **exclusive** ownership:\

```cpp
#include <memory>

//* Parent Scope block
{
  std::unique_ptr<int> a0;
  
  //* Nested scope block
  {
    std::unique_ptr<int> a1 = std::make_unique<int>(5);

    a0 = a1; //* Transfer of OWNERSHIP from `a1` to `a0`
    //* The `a1` does NOT control lifecycle of the `int` instance anymore.
    
    //* This WILL result to throwing an exception since `a1` instance
    //* is initialised to `nullptr`:
    std::cout << "a1=" << *a1 <<std::endl;
    
    //* Automatically called destructor of `a1` instance,
    //* which does NOTHING since `a1` does NOT possess
    //* control over any object.
  }
  //* At this point, The `a0` variable is still controlling lifecycle of the
  //* `int` instance created in nested scope above.
  
  //* Automatically called destructor of `a0` instance will
  //* result to destruction of the `int` instance allocated & initialised
  //* in the nested scope above.
}
```

