#ifndef KAME_UTIL_TAGGED_UNION_HELPER_H
#define KAME_UTIL_TAGGED_UNION_HELPER_H

#include <type_traits>

namespace KameUtil {

template <class T2, class ...Args2>
struct NoExcepts {
  static const bool value = noexcept(T2(std::declval<T2>())) ? 
    NoExcepts<Args2...>::value : false;
};

template <class T2>
struct NoExcepts<T2> {
  static const bool value = noexcept(T2(std::declval<T2>()));
};

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

template <class T, class U>
struct IsSameDecayed {
  static const bool value = 
    std::is_same<typename std::decay<T>::type, 
      typename std::decay<U>::type>::value;
};

} // end namespace KameUtil
#endif
