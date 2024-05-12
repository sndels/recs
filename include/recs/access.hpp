#pragma once

#include "accesses_types.hpp"
#include "component_storage.hpp"
#include <cstdint>

namespace recs
{

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class Entity
{
  public:
    Entity(ComponentStorage const &cs, EntityId id);
    ~Entity() = default;

    Entity(Entity const &) = default;
    Entity &operator=(Entity const &) = default;

    template <typename T>
        requires(Contains<ReadAccesses, T>)
    [[nodiscard]] T const &getComponent() const;

    template <typename T>
        requires(Contains<WriteAccesses, T>)
    [[nodiscard]] T &getComponent() const;

    // TODO:
    // Support structured bindings into components?
  private:
    ComponentStorage const &m_cs;
    EntityId m_id;
};

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class Query
{
  public:
    using EntityType = Entity<ReadAccesses, WriteAccesses, WithAccesses>;

    class Iterator
    {
      public:
        ~Iterator() = default;

        Iterator &operator++();
        Iterator operator++(int);
        [[nodiscard]] EntityType const &operator*() const;
        [[nodiscard]] EntityType const *operator->() const;
        [[nodiscard]] bool operator!=(Iterator const &other) const;
        [[nodiscard]] bool operator==(Iterator const &other) const;

        friend class Query;

      private:
        Iterator(Query const &query, size_t pos);

        Query const &m_query;
        size_t m_pos{0};
    };

    Query(ComponentStorage const &cs, std::vector<EntityId> const &ids)
    : m_cs{cs}
    {
        for (EntityId id : ids)
            m_entities.emplace_back(m_cs, id);
    }
    ~Query() = default;

    Query(Query const &) = default;
    Query &operator=(Query const &) = default;

    Iterator begin() const { return Iterator(*this, 0); }
    Iterator end() const { return Iterator(*this, m_entities.size()); }

    friend class Iterator;

  private:
    ComponentStorage const &m_cs;
    std::vector<EntityType> m_entities;
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

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
Entity<ReadAccesses, WriteAccesses, WithAccesses>::Entity(
    ComponentStorage const &cs, EntityId id)
: m_cs{cs}
, m_id{id}
{
    // TODO:
    // Figure out how to assert entity components here in addition to the
    // getters for an earlier error. Getting types out of accesses seems tricky.
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
template <typename T>
    requires(Contains<ReadAccesses, T>)
T const &Entity<ReadAccesses, WriteAccesses, WithAccesses>::getComponent() const
{
    assert(
        m_cs.hasComponent<T>(m_id) && "The entity is missing this component");
    return m_cs.getComponent<T>(m_id);
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
template <typename T>
    requires(Contains<WriteAccesses, T>)
T &Entity<ReadAccesses, WriteAccesses, WithAccesses>::getComponent() const
{
    assert(
        m_cs.hasComponent<T>(m_id) && "The entity is missing this component");
    return m_cs.getComponent<T>(m_id);
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
Query<ReadAccesses, WriteAccesses, WithAccesses>::Iterator::Iterator(
    Query const &query, size_t pos)
: m_query{query}
, m_pos{pos}
{
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
typename Query<ReadAccesses, WriteAccesses, WithAccesses>::Iterator &Query<
    ReadAccesses, WriteAccesses, WithAccesses>::Iterator::operator++()
{
    m_pos++;
    return *this;
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
typename Query<ReadAccesses, WriteAccesses, WithAccesses>::Iterator Query<
    ReadAccesses, WriteAccesses, WithAccesses>::Iterator::operator++(int)
{
    Iterator const ret = *this;
    m_pos++;
    return ret;
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
Entity<ReadAccesses, WriteAccesses, WithAccesses> const &Query<
    ReadAccesses, WriteAccesses, WithAccesses>::Iterator::operator*() const
{
    return m_query.m_entities[m_pos];
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
Entity<ReadAccesses, WriteAccesses, WithAccesses> const *Query<
    ReadAccesses, WriteAccesses, WithAccesses>::Iterator::operator->() const
{
    return &**this;
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
bool Query<ReadAccesses, WriteAccesses, WithAccesses>::Iterator::operator==(
    Iterator const &other) const
{
    return &this->m_query == &other.m_query && this->m_pos == other.m_pos;
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
bool Query<ReadAccesses, WriteAccesses, WithAccesses>::Iterator::operator!=(
    Iterator const &other) const
{
    return &this->m_query != &other.m_query || this->m_pos != other.m_pos;
}

} // namespace recs
