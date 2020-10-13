//
// Created by qicosmos on 2020/10/12.
//

#ifndef MESSAGE_BUS_MESSAGE_BUS_H
#define MESSAGE_BUS_MESSAGE_BUS_H
#include <unordered_map>
#include <functional>
#include <type_traits>
#include "function_traits.h"

namespace purecpp {
template <typename T>
using equality_comparison_t = decltype(std::declval<T>() == std::declval<T>());

template <typename T, typename = std::void_t<>>
struct is_equality_comparable : std::false_type {};

template <typename T>
struct is_equality_comparable<T, std::void_t<equality_comparison_t<T>>>
    : std::is_same<equality_comparison_t<T>, bool> {};

class message_bus {
public:
  static message_bus &get() {
    static message_bus instance;
    return instance;
  }

  template <typename Fn> bool register_me(const std::string &key, Fn fn) {
    if (invokers_.find(key) != invokers_.end()) {
      return false;
    }

    using Tuple = typename function_traits<Fn>::tuple_type;
    invokers_[key] = {std::bind(&invoker<Fn>::apply, fn, std::placeholders::_1,
                                std::placeholders::_2)};
    return true;
  }

  template <
      typename Fn, typename Self,
      typename = std::enable_if_t<std::is_member_function_pointer<Fn>::value>>
  bool register_me(const std::string &key, const Fn &fn, Self *t) {
    if (invokers_.find(key) != invokers_.end()) {
      return false;
    }

    using Tuple = typename function_traits<Fn>::tuple_type;
    invokers_[key] = {std::bind(&invoker<Fn>::template apply_mem<Self>, fn, t,
                                std::placeholders::_1, std::placeholders::_2)};
    return false;
  }

  template <typename R, typename... Args>
  R call(const std::string &key, Args &&... args) {
    auto it = invokers_.find(key);
    assert(it != invokers_.end());
    R result{};
    call_impl(it, &result, std::forward<Args>(args)...);
    return result;
  }

  template <typename... Args>
  void call(const std::string &key, Args &&... args) {
    auto it = invokers_.find(key);
    assert(it != invokers_.end());

    call_impl(it, nullptr, std::forward<Args>(args)...);
  }

private:
  template <typename T, typename... Args>
  void call_impl(T &it, void *ptr, Args &&... args) {
    using Tuple = decltype(std::make_tuple(std::forward<Args>(args)...));
    Tuple &&tp = std::forward_as_tuple(std::forward<Args>(args)...);
    it->second(&tp, ptr);
    update(tp, std::forward_as_tuple(std::forward<Args>(args)...),
           std::make_index_sequence<std::tuple_size<Tuple>::value>{});
  }

  template <typename T, typename U> void update_impl(T &t, U &u) {
    if constexpr (is_equality_comparable<std::decay_t<T>>::value) {
      if (u != t) {
        u = t;
      }
    }
  }

  template <typename Tuple, typename Tuple1, std::size_t... Idx>
  void update(Tuple &src, Tuple1 &&dest, std::index_sequence<Idx...>) {
    (void)std::initializer_list<int>{
        (update_impl(std::get<Idx>(src), std::get<Idx>(dest)), void(), 0)...};
  }

  template <typename Function> struct invoker {
    static inline void apply(const Function &func, void *bl, void *result) {
      using tuple_type = typename function_traits<Function>::bare_tuple_type;
      using R = typename function_traits<Function>::return_type;

      if constexpr (!std::is_void_v<R>){
        if(result== nullptr)
        assert(false);//log and throw here, because the user forgot return type
      }
      tuple_type *tp = static_cast<tuple_type *>(bl);
      call<R>(func, *tp, result);
    }

    template <typename R, typename F, typename... Args>
    static typename std::enable_if<std::is_void<R>::value>::type
    call(const F &f, std::tuple<Args...> &tp, void *) {
      call_helper<void>(f, std::make_index_sequence<sizeof...(Args)>{}, tp);
    }

    template <typename R, typename F, typename... Args>
    static typename std::enable_if<!std::is_void<R>::value>::type
    call(const F &f, std::tuple<Args...> &tp, void *result) {
      R r = call_helper<R>(f, std::make_index_sequence<sizeof...(Args)>{}, tp);
      if (result)
        *(decltype(r) *)result = r;
    }

    template <typename R, typename F, size_t... I, typename... Args>
    static R
    call_helper(const F &f, const std::index_sequence<I...> &,
                std::tuple<Args...>
                    &tup) { //-> typename std::result_of<F(Args...)>::type {
      using tuple_type = typename function_traits<Function>::tuple_type;
      return f(((std::tuple_element_t<I, tuple_type>)std::get<I>(tup))...);
    }

    template <typename TupleDest, size_t... I, typename TupleSrc>
    static inline TupleDest get_dest_tuple(std::index_sequence<I...>,
                                           TupleSrc &src) {
      return std::forward_as_tuple(
          ((std::tuple_element_t<I, TupleDest>)std::get<I>(src))...);
    }

    template <typename Self>
    static inline void apply_mem(const Function &f, Self *self, void *bl,
                                 void *result) {
      using bare_tuple_type =
          typename function_traits<Function>::bare_tuple_type;
      bare_tuple_type *tp = static_cast<bare_tuple_type *>(bl);

      using tuple_type = typename function_traits<Function>::tuple_type;
      auto dest_tp = get_dest_tuple<tuple_type>(
          std::make_index_sequence<std::tuple_size<bare_tuple_type>::value>{},
          *tp);

      using return_type = typename function_traits<Function>::return_type;
      call_mem<return_type>(
          f, self, dest_tp, result,
          std::integral_constant<bool, std::is_void<return_type>::value>{});
    }

    template <typename R, typename F, typename Self, typename... Args>
    static void call_mem(const F &f, Self *self, const std::tuple<Args...> &tp,
                         void *, std::true_type) {
      call_member_helper<void>(f, self,
                               std::make_index_sequence<sizeof...(Args)>{}, tp);
    }

    template <typename R, typename F, typename Self, typename... Args>
    static void call_mem(const F &f, Self *self, const std::tuple<Args...> &tp,
                         void *result, std::false_type) {
      auto r = call_member_helper<R>(
          f, self, std::make_index_sequence<sizeof...(Args)>{}, tp);
      if (result)
        *(R *)result = r;
    }

    template <typename R, typename F, typename Self, size_t... I,
              typename... Args>
    static R call_member_helper(
        const F &f, Self *self, const std::index_sequence<I...> &,
        const std::tuple<Args...>
            &tup) { //-> decltype((self->*f)(std::get<I>(tup)...)) {
      using tuple_type = typename function_traits<F>::tuple_type;
      return (self->*f)(
          ((std::tuple_element_t<I, tuple_type>)std::get<I>(tup))...);
    }
  };

private:
  message_bus() = default;
  message_bus(const message_bus &) = delete;
  message_bus(message_bus &&) = delete;

  std::unordered_map<std::string, std::function<void(void *, void *)>>
      invokers_;
};
}
#endif // MESSAGE_BUS_MESSAGE_BUS_H
