#ifndef TAGGED_UNION_H
#define TAGGED_UNION_H

#include <utility>
#include <stdexcept>
#include <type_traits>

template <class T, class ...Args>
class TaggedUnion {
  template <class U>
  struct IsNotSame {
    static const bool value = 
      !std::is_same<typename std::decay<U>::type, TaggedUnion>::value;
  };

  template <class T2, class ...Args2>
  struct NoExcepts {
    static const bool value = noexcept(T2(std::declval<T2>())) ? 
      NoExcepts<Args2...>::value : false;
  };

  template <class T2>
  struct NoExcepts<T2> {
    static const bool value = noexcept(T2(std::declval<T2>()));
  };

public:
  template <class U,
    typename std::enable_if<IsNotSame<U>::value>::type* = nullptr>
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
  auto call(Func &&f)
  {
    return call(std::forward<Func>(f), Dummy<T, Args...>());  
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
  auto call(Func &&f, Dummy<T2>)
  {
    if (tag == 0) {
      T2 *t2 = (T2*)data;
      return f(*t2);
    } else {
      throw std::runtime_error("Union is in an invalid state");
    }
  }

  template <class Func, class T2, class ...Args2>
  auto call(Func &&f, Dummy<T2, Args2...>)
  {
    if (tag == sizeof...(Args2)) {
      T2 *t2 = (T2*)data;
      return f(*t2);
    } else {
      return call(std::forward<Func>(f), Dummy<Args2...>());
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

  template <int size, class T2, class ...Args2>
  struct MaxSize {
    static const int value = size < sizeof(T2) ? 
      MaxSize<sizeof(T2), Args2...>::value : 
      MaxSize<size, Args2...>::value;
  };

  template <int size, class T2>
  struct MaxSize<size, T2> {
    static const int value = size < sizeof(T2) ? sizeof(T2) : size;
  };

  template <int align, class T2, class ...Args2>
  struct MaxAlign {
    static const int value = align < alignof(T2) ? 
      MaxAlign<alignof(T2), Args2...>::value : 
      MaxAlign<align, Args2...>::value;
  };

  template <int align, class T2>
  struct MaxAlign<align, T2> {
    static const int value = align < alignof(T2) ? alignof(T2) : align;
  };

  alignas(MaxAlign<alignof(T), Args...>::value) 
    char data[MaxSize<sizeof(T), Args...>::value];
  int tag;
};

#endif
