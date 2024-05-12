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
    template <typename T>
        requires(SameAs<T, Ts> || ...)
    static consteval bool contains()
    {
        return true;
    }

    template <typename T> static consteval bool contains() { return false; }
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
