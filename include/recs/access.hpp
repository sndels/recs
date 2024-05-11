#pragma once

#include "accesses_types.hpp"
#include <cstdint>

namespace recs
{

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class Entity
{
  public:
    template <typename T>
        requires(Contains<ReadAccesses, T>)
    T const &getComponent()
    {
        // TODO: actual data here
        T const *ptr = nullptr;

        return *ptr;
    }

    template <typename T>
        requires(Contains<WriteAccesses, T>)
    T &getComponent()
    {
        // TODO: actual data here
        T *ptr = nullptr;

        return *ptr;
    }

    // TODO:
    // Support structured bindings into components?
};

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class Query
{
  public:
    // TODO:
    // This probably needs a list of IDs or something internally and an iterator
    // type that dereferences as Entity

    Entity<ReadAccesses, WriteAccesses, WithAccesses> *begin()
    {
        // TODO: Actual entities
        return nullptr;
    }
    Entity<ReadAccesses, WriteAccesses, WithAccesses> *end()
    {
        // TODO: Actual entities
        return nullptr;
    }

    Entity<ReadAccesses, WriteAccesses, WithAccesses> const *begin() const
    {
        // TODO: Actual entities
        return nullptr;
    }
    Entity<ReadAccesses, WriteAccesses, WithAccesses> const *end() const
    {
        // TODO: Actual entities
        return nullptr;
    }
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
