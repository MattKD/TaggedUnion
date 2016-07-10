#ifndef TAGGED_UNION_H
#define TAGGED_UNION_H

#include <utility>
#include <stdexcept>

template <class T, class ...Args>
class TaggedUnion {
public:
  template <class U>
  TaggedUnion(const U &val) : tag{GetTag<U, T, Args...>::tag}
  {
    new (data) U(val);
  }

  TaggedUnion(const TaggedUnion &u)
  {
    copy(u, Dummy<T, Args...>()); 
  }

  TaggedUnion(TaggedUnion &&u)
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
  decltype(auto) call(Func f)
  {
    return call(f, Dummy<T, Args...>());  
  }

  template <class U>
  void reset(const U &val)
  {
    release(Dummy<T, Args...>());
    new (data) U(val);
    tag = GetTag<U, T, Args...>::tag;
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
      return *(U*)data;
    } else {
      throw std::runtime_error("Union:get<T> called with wrong current type");
    }
  }

  template <class U>
  U& unsafeGet() const
  {
    return *(U*)data;
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
  decltype(auto) call(Func &f, Dummy<T2>)
  {
    if (tag == 0) {
      return f(*(T2*)data);
    } else {
      throw std::runtime_error("Union is in an invalid state");
    }
  }

  template <class Func, class T2, class ...Args2>
  decltype(auto) call(Func &f, Dummy<T2, Args2...>)
  {
    if (tag == sizeof...(Args2)) {
      return f(*(T2*)data);
    } else {
      return call(f, Dummy<Args2...>());
    }
  }

  template <class T2>
  void release(Dummy<T2>)
  {
    if (tag == 0) {
      ((T2*)data)->~T2();
      tag = -1; // invalid state
    }
  }

  template <class T2, class ...Args2>
  void release(Dummy<T2, Args2...>)
  {
    if (tag == sizeof...(Args2)) {
      ((T2*)data)->~T2();
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
