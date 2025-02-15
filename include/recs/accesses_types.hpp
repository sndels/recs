#pragma once

#include "component_mask.hpp"
#include "concepts.hpp"
#include "type_id.hpp"

namespace recs
{

template <typename... Ts> class AccessesType;

template <> class AccessesType<>
{
  public:
    template <typename T> static consteval bool contains() { return false; }
    static void setMask(ComponentMask &) { }
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

    static void setMask(ComponentMask &mask)
    {
        (void(mask.set(TypeId::get<Ts>())), ...);
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

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class AccessBuilder
{
  public:
    template <typename T>
    using Read = AccessBuilder<
        typename ReadAccesses::template Append<T>, WriteAccesses, WithAccesses>;

    template <typename T>
    using Write = AccessBuilder<
        ReadAccesses, typename WriteAccesses::template Append<T>, WithAccesses>;

    template <typename T>
    using With = AccessBuilder<
        ReadAccesses, WriteAccesses, typename WithAccesses::template Append<T>>;

    template <template <
        typename EntityReads, typename EntityWrites, typename EntityWiths>
              typename T>
    using As = T<ReadAccesses, WriteAccesses, WithAccesses>;
};

using Access =
    AccessBuilder<ReadAccessesType<>, WriteAccessesType<>, WithAccessesType<>>;

} // namespace recs
