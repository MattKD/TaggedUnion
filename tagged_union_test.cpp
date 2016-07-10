#include "tagged_union.h"
#include <assert.h>
#include <string>

static int Foo_count = 0;
struct Foo {
  Foo(int a, int b) : a{a}, b{b} { ++Foo_count; }
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

int main()
{
  using std::string;

  TaggedUnion<char, double, Foo, std::string, int> u(8.5);

  // u.reset(short(1)); // compile error
  // u.get<short>() = 7; // exception thrown
  // u.unsafeGet<short>() = 7; // undefined behavior

  assert(u.call(Func()) == DoubleType);
  assert(u.isType<double>());
  assert(u.get<double>() == 8.5);
  assert(u.unsafeGet<double>() == 8.5);

  u.reset(5);
  assert(u.call(Func()) == IntType);
  assert(u.isType<int>());
  assert(u.get<int>() == 5);
  assert(u.unsafeGet<int>() == 5);

  assert(Foo_count == 0);
  u.reset(Foo(2, 3));
  assert(Foo_count == 1); // incremented in Foo ctor
  assert(u.call(Func()) == FooType);
  assert(u.isType<Foo>());
  assert(u.get<Foo>().a == 2 && u.get<Foo>().b == 3);
  assert(u.unsafeGet<Foo>().a == 2 && u.unsafeGet<Foo>().b == 3);


  u.reset(string("hello world"));
  assert(Foo_count == 0); // decremented in Foo dtor
  assert(u.call(Func()) == StringType);
  assert(u.isType<string>());
  assert(u.get<string>() == string("hello world"));
  assert(u.unsafeGet<string>() == string("hello world"));

  //TaggedUnion<char, double, Foo, string, int> u2(std::move(u));
  auto u2 = std::move(u);
  assert(u.call(Func()) == StringType);
  assert(u.isType<string>());
  assert(u.get<string>() == string());
  assert(u.unsafeGet<string>() == string());
  assert(u2.call(Func()) == StringType);
  assert(u2.isType<string>());
  assert(u2.get<string>() == string("hello world"));
  assert(u2.unsafeGet<string>() == string("hello world"));

  u = std::move(u2);
  assert(u.call(Func()) == StringType);
  assert(u.isType<string>());
  assert(u.get<string>() == string("hello world"));
  assert(u.unsafeGet<string>() == string("hello world"));
  assert(u2.call(Func()) == StringType);
  assert(u2.isType<string>());
  assert(u2.get<string>() == string());
  assert(u2.unsafeGet<string>() == string());

  u2 = u;
  assert(u.call(Func()) == StringType);
  assert(u.isType<string>());
  assert(u.get<string>() == string("hello world"));
  assert(u.unsafeGet<string>() == string("hello world"));
  assert(u2.call(Func()) == StringType);
  assert(u2.isType<string>());
  assert(u2.get<string>() == string("hello world"));
  assert(u2.unsafeGet<string>() == string("hello world"));

  try {
    u.get<int>() = 0; // will throw
    assert(false);
  } catch (...) {  }

  try {
    // exception in copy ctor, leaving u in an invalid state
    u.reset(Foo(-1,-1)); 
    assert(false);
  } catch (...) {  }

  try {
    u.call(Func()); // trying to call in an invalid state, so will throw
    assert(false);
  } catch (...) {  }

  return 0;
}

