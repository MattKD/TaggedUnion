#ifndef KAME_UTIL_TAGGED_UNION_H
#define KAME_UTIL_TAGGED_UNION_H

#include "tagged_union_helper.h"
#include <utility>
#include <stdexcept>

namespace KameUtil {

template <class T, class ...Args>
class TaggedUnion {
public:
  template <class U,
    typename std::enable_if<!IsSameDecayed<U, TaggedUnion>::value>::type* 
      = nullptr>
  explicit TaggedUnion(U &&val)
    : tag{GetTag<typename std::remove_reference<U>::type, T, Args...>::tag}
  {
    new (data) typename std::remove_reference<U>::type(std::forward<U>(val));
  }

  TaggedUnion(const TaggedUnion &u)
  {
    copy(u, Dummy<T, Args...>()); 
  }

  TaggedUnion(TaggedUnion &&u) noexcept(NoExcepts<T, Args...>::value)
  {
    move(std::move(u), Dummy<T, Args...>()); 
  }

  TaggedUnion& operator=(const TaggedUnion &u)
  {
    if (this != &u) {
      release(Dummy<T, Args...>());
      copy(u, Dummy<T, Args...>()); 
    }
    return *this;
  }

  TaggedUnion& operator=(TaggedUnion &&u)
  {
    if (this != &u) {
      release(Dummy<T, Args...>());
      move(std::move(u), Dummy<T, Args...>()); 
    }
    return *this;
  }

  ~TaggedUnion()
  {
    release(Dummy<T, Args...>());
  }

  template <class Func>
  auto apply(Func &&f)
  {
    return apply(std::forward<Func>(f), Dummy<T, Args...>());  
  }

  template <class U>
  void reset(U &&val)
  {
    release(Dummy<T, Args...>());
    new (data) typename std::remove_reference<U>::type(std::forward<U>(val));
    tag = GetTag<typename std::remove_reference<U>::type, T, Args...>::tag;
  }

  template <class U>
  bool isType() const
  {
    return tag == GetTag<U, T, Args...>::tag;
  }

  template <class U>
  U& get() const
  {
    if (tag == GetTag<U, T, Args...>::tag) {
      U *u = (U*)data;
      return *u;
    } else {
      throw std::runtime_error("Union:get<T> called with wrong current type");
    }
  }

  template <class U>
  U& unsafeGet() const
  {
    U *u = (U*)data;
    return *u;
  }

  bool isValid() const { return tag >= 0; }

private:
  template <class U, class ...Args2>
  struct GetTag {
    template <class T2, class ...Args3>
    struct GetTag2 {
      static_assert(sizeof...(Args3) != 0, "Tried to set Union to an invalid type");
      static const int tag = GetTag2<Args3...>::tag;
    };

    template <class ...Args3>
    struct GetTag2<U, Args3...> {
      static const int tag = sizeof...(Args3);
    };

    static const int tag = GetTag2<Args2...>::tag;
  };

  template <class T2, class ...Args2>
  struct Dummy { };

  template <class T2>
  void copy(const TaggedUnion &u, Dummy<T2>)
  {
    if (u.tag == 0) { // Last type listed has tag 0
      new(data) T2(u.unsafeGet<T2>());
      tag = u.tag;
    } else {
      throw std::runtime_error("Union is in an invalid state");
    }
  }

  template <class T2, class ...Args2>
  void copy(const TaggedUnion &u, Dummy<T2, Args2...>)
  {
    // Type's tag is equal to number of types listed after it
    if (u.tag == sizeof...(Args2)) { 
      new(data) T2(u.unsafeGet<T2>());
      tag = u.tag;
    } else {
      copy(u, Dummy<Args2...>());
    }
  }

  template <class T2>
  void move(TaggedUnion &&u, Dummy<T2>)
  {
    if (u.tag == 0) { // Last type listed has tag 0
      new(data) T2(std::move(u.unsafeGet<T2>()));
      tag = u.tag;
    } else {
      throw std::runtime_error("Union is in an invalid state");
    }
  }

  template <class T2, class ...Args2>
  void move(TaggedUnion &&u, Dummy<T2, Args2...>)
  {
    // Type's tag is equal to number of types listed after it
    if (u.tag == sizeof...(Args2)) { 
      new(data) T2(std::move(u.unsafeGet<T2>()));
      tag = u.tag;
    } else {
      move(std::move(u), Dummy<Args2...>());
    }
  }

  template <class Func, class T2>
  auto apply(Func &&f, Dummy<T2>)
  {
    if (tag == 0) {
      T2 *t2 = (T2*)data;
      return f(*t2);
    } else {
      throw std::runtime_error("Union is in an invalid state");
    }
  }

  template <class Func, class T2, class ...Args2>
  auto apply(Func &&f, Dummy<T2, Args2...>)
  {
    if (tag == sizeof...(Args2)) {
      T2 *t2 = (T2*)data;
      return f(*t2);
    } else {
      return apply(std::forward<Func>(f), Dummy<Args2...>());
    }
  }

  template <class T2>
  void release(Dummy<T2>)
  {
    if (tag == 0) {
      T2 *t2 = (T2*)data;
      t2->~T2();
      tag = -1; // invalid state
    }
  }

  template <class T2, class ...Args2>
  void release(Dummy<T2, Args2...>)
  {
    if (tag == sizeof...(Args2)) {
      T2 *t2 = (T2*)data;
      t2->~T2();
      tag = -1; // invalid state
    } else {
      release(Dummy<Args2...>());
    }
  }

  alignas(MaxAlign<alignof(T), Args...>::value) 
    char data[MaxSize<sizeof(T), Args...>::value];
  int tag;
};

} // end namespace KameUtil
#endif
