#pragma once

#include "concepts.hpp"

namespace recs
{

template <typename... Ts> class AccessesType;

template <> class AccessesType<>
{
  public:
    template <typename T> static consteval bool contains() { return false; }
};
template <typename... Ts> class AccessesType
{
  public:
    template <typename T> static consteval bool contains()
    {
        return containsImpl<T, Ts...>();
    }

  private:
    template <typename T> static consteval bool containsImpl() { return false; }

    template <typename T, typename First, typename... Rest>
        requires(SameAs<T, First>)
    static consteval bool containsImpl()
    {
        return true;
    }

    template <typename T, typename First, typename... Rest>
    static consteval bool containsImpl()
    {
        return containsImpl<T, Rest...>();
    }
};

template <typename... Ts> class ReadAccessesType : public AccessesType<Ts...>
{
  public:
    template <typename T> using Append = ReadAccessesType<T, Ts...>;
};
template <> class ReadAccessesType<> : public AccessesType<>
{
  public:
    template <typename T> using Append = ReadAccessesType<T>;
};

template <typename... Ts> class WriteAccessesType : public AccessesType<Ts...>
{
  public:
    template <typename T> using Append = WriteAccessesType<T, Ts...>;
};
template <> class WriteAccessesType<> : public AccessesType<>
{
  public:
    template <typename T> using Append = WriteAccessesType<T>;
};

template <typename... Ts> class WithAccessesType : public AccessesType<Ts...>
{
  public:
    template <typename T> using Append = WithAccessesType<T, Ts...>;
};
template <> class WithAccessesType<> : public AccessesType<>
{
  public:
    template <typename T> using Append = WithAccessesType<T>;
};

} // namespace recs
