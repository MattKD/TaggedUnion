#include "tagged_union.h"
#include <iostream>
#include <chrono>
#include <vector>


struct UnionA {
  int foo(int n) { return n + 1; }
};
struct UnionB {
  int foo(int n) { return n * 2; }
};
struct UnionC {
  int foo(int n) { return n / 2; }
};

struct FuncFoo {
  FuncFoo(int n) : n{n} { }
  int operator()(UnionA *u) { return u->foo(n); }
  int operator()(UnionB *u) { return u->foo(n); }
  int operator()(UnionC *u) { return u->foo(n); }
  int n;
};

struct FuncDelete {
  void operator()(UnionA *u) { delete u; }
  void operator()(UnionB *u) { delete u; }
  void operator()(UnionC *u) { delete u; }
};

void taggedUnionPerfTest(int num_objects)
{
  using std::vector;
  using std::cout;
  using std::endl;
  typedef TaggedUnion<UnionA*, UnionB*, UnionC*> UnionT;
  std::vector<UnionT> unions;

  for (int i = 0; i < num_objects; ++i) {
    if (i % 3 == 0) {
      unions.push_back(UnionT(new UnionA())); 
    } else if (i % 3 == 1) {
      unions.push_back(UnionT(new UnionB())); 
    } else {
      unions.push_back(UnionT(new UnionC())); 
    }
  }

  auto t1 = std::chrono::high_resolution_clock::now();

  int n = 1;
  for (UnionT &u : unions) {
    n += u.apply(FuncFoo(n));
  }

  auto t2 = std::chrono::high_resolution_clock::now();

  for (UnionT &u : unions) {
    u.apply(FuncDelete());
  }
  std::chrono::duration<double, std::milli> duration = t2 - t1;
  cout << "taggedUnionPerfTest: " << duration.count() << "ms\n"; 
}

enum UnionTag {
  UnionAType,
  UnionBType,
  UnionCType
};

struct UnsafeUnion {
  UnionTag tag;
  union {
    UnionA *a;
    UnionB *b;
    UnionC *c;
  };
};

void unsafeUnionPerfTest(int num_objects)
{
  using std::vector;
  using std::cout;
  using std::endl;
  std::vector<UnsafeUnion> unions;

  for (int i = 0; i < num_objects; ++i) {
    if (i % 3 == 0) {
      UnsafeUnion u;
      u.tag = UnionAType;
      u.a = new UnionA();
      unions.push_back(u); 
    } else if (i % 3 == 1) {
      UnsafeUnion u;
      u.tag = UnionBType;
      u.b = new UnionB();
      unions.push_back(u); 
    } else {
      UnsafeUnion u;
      u.tag = UnionCType;
      u.c = new UnionC();
      unions.push_back(u); 
    }
  }

  auto t1 = std::chrono::high_resolution_clock::now();

  int n = 1;
  for (UnsafeUnion &u : unions) {
    switch (u.tag) {
    case UnionAType:
      n += u.a->foo(n);
      break;
    case UnionBType:
      n += u.b->foo(n);
      break;
    case UnionCType:
      n += u.c->foo(n);
      break;
    }
  }

  auto t2 = std::chrono::high_resolution_clock::now();

  for (UnsafeUnion &u : unions) {
    switch (u.tag) {
    case UnionAType:
      delete u.a;
      break;
    case UnionBType:
      delete u.b;
      break;
    case UnionCType:
      delete u.c;
      break;
    }
  }

  std::chrono::duration<double, std::milli> duration = t2 - t1;
  cout << "unsafeUnionPerfTest: " << duration.count() << "ms\n"; 
}


struct Base {
  virtual int foo(int n) = 0;
  virtual ~Base() {};
};
struct SubtypeA : Base {
  int foo(int n) override { return n + 1; }
};
struct SubtypeB : Base {
  int foo(int n) override{ return n * 2; }
};
struct SubtypeC : Base {
  int foo(int n) override { return n / 2; }
};

void subtypePerfTest(int num_objects)
{
  using std::vector;
  using std::cout;
  using std::endl;
  std::vector<Base*> unions;

  for (int i = 0; i < num_objects; ++i) {
    if (i % 3 == 0) {
      unions.push_back(new SubtypeA()); 
    } else if (i % 3 == 1) {
      unions.push_back(new SubtypeB()); 
    } else {
      unions.push_back(new SubtypeC()); 
    }
  }

  auto t1 = std::chrono::high_resolution_clock::now();

  int n = 1;
  for (Base *b : unions) {
    n += b->foo(n);
  }

  auto t2 = std::chrono::high_resolution_clock::now();

  for (Base *b : unions) {
    delete b;
  }

  std::chrono::duration<double, std::milli> duration = t2 - t1;
  cout << "SubtypePerfTest: " << duration.count() << "ms\n"; 
}


