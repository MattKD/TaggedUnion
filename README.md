# TaggedUnion
Safe tagged union template class in C++14

This is my attempt at a safe tagged union in C++14. It passes all the included tests checking that the correct values are stored and returned, destructors are called, exceptions thrown when attempting to read a wrong value, and that it works with move only types. It is left in an invalid state if copy/move ctor throws in the reset method. Performance is better than virtual functions, but worse than plain unions with switches in simple test cases.

Basic Example:
```
#include "tagged_union.h"
#include <memory>
#include <iostream>
#include <string>

using std::unique_ptr;
using std::cout;
using std::string;
using KameUtil::TaggedUnion;

struct Func {
  void operator()(int n) { cout << n << "\n"; }
  void operator()(const string &s) { cout << s << "\n"; }
  void operator()(const unique_ptr<double> &uptr) { cout << *uptr << "\n"; }
};

int main()
{
  cout << std::boolalpha;

  TaggedUnion<int, string, unique_ptr<double>> u(string("hello world"));
  cout << u.isValid() << "\n";
  cout << u.isType<string>() << "\n";
  u.apply(Func()); // prints "hello world"

  // implicit conversion not supported
  //TaggedUnion<int, string, unique_ptr<Foo>> u("hello world"); 

  auto u2 = std::move(u);
  cout << u.isValid() << "\n";
  cout << u.isType<string>() << "\n";
  cout << u2.isValid() << "\n";
  cout << u2.isType<string>() << "\n";

  //auto u3 = u2; // copy ctor won't work with unique_ptr

  u.reset(5);
  cout << u.isValid() << "\n";
  cout << u.isType<int>() << "\n";
  u.apply(Func()); // prints 5

  //u.reset(5u); // implicit conversion not supported

  u.reset(unique_ptr<double>(new double(3.41)));
  // all print 3.41
  u.apply(Func());
  cout << *(u.get<unique_ptr<double>>()) << "\n";
  cout << *(u.unsafeGet<unique_ptr<double>>()) << "\n";

  // get doesn't compile with unused type
  //cout << u.get<char>() << "\n";
  // get throws exception with wrong current type
  //cout << u.get<int>() << "\n";
  // unsafeGet has undefined behavior with wrong current type
  //cout << u.unsafeGet<int>() << "\n";

  return 0;
}
```
