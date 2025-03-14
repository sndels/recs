#pragma once

#include "accesses_types.hpp"
#include "naive_component_storage.hpp"
#include "type_id.hpp"
#include <cstdint>

namespace recs::naive
{

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class QueryIterator;

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class Entity
{
  public:
    Entity(ComponentStorage const &cs, EntityId id);
    // Copy range and position to permit having multiple entities around from a
    // single Query
    Entity(ComponentStorage::Range const &range, size_t pos);
    Entity(ComponentStorage::Range const &range);
    ~Entity() = default;

    Entity(Entity const &) = default;
    Entity(Entity &&) = default;
    Entity &operator=(Entity const &) = default;
    Entity &operator=(Entity &&) = default;

    template <typename T>
        requires(Contains<ReadAccesses, T>)
    [[nodiscard]] T const &getComponent() const;

    template <typename T>
        requires(Contains<WriteAccesses, T>)
    [[nodiscard]] T &getComponent() const;

    [[nodiscard]] static ComponentMask accessMask()
    {
        ComponentMask mask;

        ReadAccesses::setMask(mask);
        WriteAccesses::setMask(mask);
        WithAccesses::setMask(mask);

        return mask;
    }

    [[nodiscard]] static ComponentMask writeAccessMask()
    {
        ComponentMask mask;

        WriteAccesses::setMask(mask);

        return mask;
    }

    // TODO:
    // Support structured bindings into components?
  private:
    ComponentStorage const *m_cs{nullptr};
    EntityId m_id;
};

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class Query;

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class QueryIterator
{
  public:
    using EntityType = Entity<ReadAccesses, WriteAccesses, WithAccesses>;

    ~QueryIterator() = default;

    QueryIterator(QueryIterator const &) = default;
    QueryIterator(QueryIterator &&) = default;
    QueryIterator &operator=(QueryIterator const &) = default;
    QueryIterator &operator=(QueryIterator &&) = default;

    QueryIterator &operator++();
    // Returns by value instead of by ref to avoid confusion when ++ invalidates
    // the previous returned ref
    // TODO:
    // What are the usual rules, can operator++ invalidate results from previous
    // iterator dereference?
    [[nodiscard]] EntityType operator*() const;
    [[nodiscard]] EntityType const *operator->() const;
    [[nodiscard]] bool operator!=(QueryIterator const &other) const;
    [[nodiscard]] bool operator==(QueryIterator const &other) const;

    friend class Query<ReadAccesses, WriteAccesses, WithAccesses>;

  private:
    QueryIterator(ComponentStorage::Range const &range, size_t pos);
    QueryIterator(ComponentStorage::Range const &range);
    QueryIterator() = default;

    ComponentStorage::Range const *m_range{nullptr};
    size_t m_pos{0};
    EntityType m_current_entity;
};

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class Query
{
  public:
    using EntityType = Entity<ReadAccesses, WriteAccesses, WithAccesses>;

    Query(ComponentStorage::Range &&range)
    : m_range{std::move(range)}
    {
    }

    ~Query() = default;

    Query(Query const &) = default;
    Query(Query &&) = delete;
    Query &operator=(Query const &) = default;
    Query &operator=(Query &&) = delete;

    QueryIterator<ReadAccesses, WriteAccesses, WithAccesses> begin() const
    {
        return QueryIterator<ReadAccesses, WriteAccesses, WithAccesses>(
            m_range, 0);
    }
    QueryIterator<ReadAccesses, WriteAccesses, WithAccesses> end() const
    {
        return QueryIterator<ReadAccesses, WriteAccesses, WithAccesses>(
            m_range);
    }

    [[nodiscard]] static ComponentMask accessMask()
    {
        ComponentMask mask;

        ReadAccesses::setMask(mask);
        WriteAccesses::setMask(mask);
        WithAccesses::setMask(mask);

        return mask;
    }

    [[nodiscard]] static ComponentMask writeAccessMask()
    {
        ComponentMask mask;

        WriteAccesses::setMask(mask);

        return mask;
    }

    friend class Iterator;

  private:
    ComponentStorage::Range m_range;
};

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
Entity<ReadAccesses, WriteAccesses, WithAccesses>::Entity(
    ComponentStorage const &cs, EntityId id)
: m_cs{&cs}
, m_id{id}
{
    // TODO:
    // Figure out how to assert entity components here in addition to the
    // getters for an earlier error. Getting types out of accesses seems tricky.
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
Entity<ReadAccesses, WriteAccesses, WithAccesses>::Entity(
    ComponentStorage::Range const &range, size_t pos)
: m_cs{&range.m_cs}
, m_id{range.m_entities[pos]}
{
    // TODO:
    // Figure out how to assert entity components here in addition to the
    // getters for an earlier error. Getting types out of accesses seems tricky.
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
Entity<ReadAccesses, WriteAccesses, WithAccesses>::Entity(
    ComponentStorage::Range const &range)
: m_cs{&range.m_cs}
{
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
template <typename T>
    requires(Contains<ReadAccesses, T>)
T const &Entity<ReadAccesses, WriteAccesses, WithAccesses>::getComponent() const
{
    assert(m_cs != nullptr);
    assert(
        m_cs->hasComponent<T>(m_id) && "The entity is missing this component");
    return m_cs->getComponent<T>(m_id);
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
template <typename T>
    requires(Contains<WriteAccesses, T>)
T &Entity<ReadAccesses, WriteAccesses, WithAccesses>::getComponent() const
{
    assert(m_cs != nullptr);
    assert(
        m_cs->hasComponent<T>(m_id) && "The entity is missing this component");
    return m_cs->getComponent<T>(m_id);
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
QueryIterator<ReadAccesses, WriteAccesses, WithAccesses>::QueryIterator(
    ComponentStorage::Range const &range, size_t pos)
: m_range{&range}
, m_pos{pos}
, m_current_entity{*m_range, m_pos}
{
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
QueryIterator<ReadAccesses, WriteAccesses, WithAccesses>::QueryIterator(
    ComponentStorage::Range const &range)
: m_range{&range}
, m_pos{range.size()}
, m_current_entity{*m_range}
{
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
QueryIterator<ReadAccesses, WriteAccesses, WithAccesses> &QueryIterator<
    ReadAccesses, WriteAccesses, WithAccesses>::operator++()
{
    assert(m_range != nullptr);
    assert(m_pos < m_range->size());
    m_pos++;
    if (m_pos < m_range->size())
        m_current_entity = EntityType{*m_range, m_pos};
    else
        m_current_entity = EntityType{*m_range};
    return *this;
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
Entity<ReadAccesses, WriteAccesses, WithAccesses> QueryIterator<
    ReadAccesses, WriteAccesses, WithAccesses>::operator*() const
{
    return m_current_entity;
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
Entity<ReadAccesses, WriteAccesses, WithAccesses> const *QueryIterator<
    ReadAccesses, WriteAccesses, WithAccesses>::operator->() const
{
    return &m_current_entity;
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
bool QueryIterator<ReadAccesses, WriteAccesses, WithAccesses>::operator==(
    QueryIterator const &other) const
{
    assert(
        m_range == other.m_range && "Comparing iterators to different ranges");

    bool const ret = m_pos == other.m_pos;
    return ret;
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
bool QueryIterator<ReadAccesses, WriteAccesses, WithAccesses>::operator!=(
    QueryIterator const &other) const
{
    assert(
        m_range == other.m_range && "Comparing iterators to different ranges");

    bool const ret = m_pos != other.m_pos;
    return ret;
}

} // namespace recs::naive
