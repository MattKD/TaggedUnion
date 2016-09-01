#include "tagged_union.h"
#include <string>
#include <iostream>
#include <memory>
#include <vector>

static int Foo_count = 0;
struct Foo {
  Foo(int a, int b) : a{a}, b{b} { ++Foo_count; }
  Foo(Foo &&f) : a{f.a}, b{f.b} 
  { 
    f.a = f.b = 0; 
    ++Foo_count; 
  }
  Foo(const Foo &f) : a{f.a}, b{f.b} 
  { 
    if (a == -1 && b == -1) {
      throw std::runtime_error("Foo ctor exception"); 
    }
    ++Foo_count;
  }

  ~Foo() { --Foo_count; }
  int a, b;
};

enum TypeResult {
  IntType,
  CharType,
  DoubleType,
  FooType,
  StringType
};

struct Func {
  TypeResult operator()(int n) { return IntType; }
  TypeResult operator()(char c) { return CharType; }
  TypeResult operator()(double d) { return DoubleType; }
  TypeResult operator()(Foo &f) { return FooType; }
  TypeResult operator()(std::string &s) { return StringType; }
};

static bool error_found = false;
void logError_(bool expr, const char *msg, const char *file, int line)
{
  if (!expr) {
    std::cerr << "Error: " << msg << ", in file " << file 
      << ", line " << line << "\n";
    error_found = true;
  }
}

#define logError(expr) logError_(expr, #expr, __FILE__, __LINE__)

void errorTest()
{
  using std::string;

  {
    // does not compile with move only types (unique_ptr)
    /*
    using std::unique_ptr;
    TaggedUnion<unique_ptr<int>, unique_ptr<double>> 
      u(unique_ptr<int>(new int(3)));
    auto u2 = std::move(u);
    */

    // doesnt' compile because move ctor is not noexcept
    //std::vector<TaggedUnion<unique_ptr<int>, unique_ptr<double>>> us; 

    // check if can compile with lvalue ref to ctor
    Foo f(1,2);
    TaggedUnion<char, double, Foo, string, int> u3(f);
  }


  error_found = false;
  TaggedUnion<char, double, Foo, string, int> u(8.5);

  size_t max_align = alignof(char);
  if (max_align < alignof(double)) max_align = alignof(double);
  if (max_align < alignof(Foo)) max_align = alignof(Foo);
  if (max_align < alignof(string)) max_align = alignof(string);
  if (max_align < alignof(int)) max_align = alignof(int);
  logError(max_align == alignof(decltype(u)));
  logError(sizeof(u) % max_align == 0);

  size_t max_size = sizeof(char);
  if (max_size < sizeof(double)) max_size = sizeof(double);
  if (max_size < sizeof(Foo)) max_size = sizeof(Foo);
  if (max_size < sizeof(string)) max_size = sizeof(string);
  if (max_size < sizeof(int)) max_size = sizeof(int);
  size_t expected_size = max_size + sizeof(int);
  if (expected_size % max_align != 0) {
    expected_size += max_align - expected_size % max_align;
  }
  logError(expected_size == sizeof(u));


  // u.reset(short(1)); // compile error
  // u.get<short>() = 7; // exception thrown
  // u.unsafeGet<short>() = 7; // undefined behavior

  logError(u.call(Func()) == DoubleType);
  logError(u.isType<double>());
  logError(u.get<double>() == 8.5);
  logError(u.unsafeGet<double>() == 8.5);

  u.reset(5);
  logError(u.call(Func()) == IntType);
  logError(u.isType<int>());
  logError(u.get<int>() == 5);
  logError(u.unsafeGet<int>() == 5);

  logError(Foo_count == 0);
  u.reset(Foo(2, 3));
  logError(Foo_count == 1); // incremented in Foo ctor
  logError(u.call(Func()) == FooType);
  logError(u.isType<Foo>());
  logError(u.get<Foo>().a == 2 && u.get<Foo>().b == 3);
  logError(u.unsafeGet<Foo>().a == 2 && u.unsafeGet<Foo>().b == 3);

  u.reset(string("hello world"));
  logError(Foo_count == 0); // decremented in Foo dtor
  logError(u.call(Func()) == StringType);
  logError(u.isType<string>());
  logError(u.get<string>() == string("hello world"));
  logError(u.unsafeGet<string>() == string("hello world"));

  u.reset(Foo(5,4));
  auto u2 = std::move(u); // u's Foo.x and Foo.y should be 0 after move
  logError(u.call(Func()) == FooType);
  logError(u.isType<Foo>());
  logError(u.get<Foo>().a == 0 && u.get<Foo>().b == 0);
  logError(u.unsafeGet<Foo>().a == 0 && u.unsafeGet<Foo>().b == 0);
  logError(u2.call(Func()) == FooType);
  logError(u2.isType<Foo>());
  logError(u2.get<Foo>().a == 5 && u2.get<Foo>().b == 4);
  logError(u2.unsafeGet<Foo>().a == 5 && u2.unsafeGet<Foo>().b == 4);

  u = std::move(u2); // u2's Foo.x and Foo.y should be 0 after move
  logError(u.call(Func()) == FooType);
  logError(u.isType<Foo>());
  logError(u.get<Foo>().a == 5 && u.get<Foo>().b == 4);
  logError(u.unsafeGet<Foo>().a == 5 && u.unsafeGet<Foo>().b == 4);
  logError(u2.call(Func()) == FooType);
  logError(u2.isType<Foo>());
  logError(u2.get<Foo>().a == 0 && u2.get<Foo>().b == 0);
  logError(u2.unsafeGet<Foo>().a == 0 && u2.unsafeGet<Foo>().b == 0);

  u2 = u; // u and u2's Foo.x and Foo.y should be 5,4 after assign
  logError(u.call(Func()) == FooType);
  logError(u.isType<Foo>());
  logError(u.get<Foo>().a == 5 && u.get<Foo>().b == 4);
  logError(u.unsafeGet<Foo>().a == 5 && u.unsafeGet<Foo>().b == 4);
  logError(u2.call(Func()) == FooType);
  logError(u2.isType<Foo>());
  logError(u2.get<Foo>().a == 5 && u2.get<Foo>().b == 4);
  logError(u2.unsafeGet<Foo>().a == 5 && u2.unsafeGet<Foo>().b == 4);

  try {
    u.get<int>() = 0; // will throw
    logError(false);
  } catch (...) {  }

  try {
    // exception in copy ctor, leaving u in an invalid state
    Foo f(-1, -1);
    u.reset(f); 
    logError(false);
  } catch (...) {  }

  try {
    u.call(Func()); // trying to call in an invalid state, so will throw
    logError(false);
  } catch (...) {  }

  if (!error_found) {
    std::cout << "All error tests passed\n";
  }
}

