#pragma once

#include "accesses_types.hpp"
#include "component_storage.hpp"
#include "type_id.hpp"
#include <cstdint>

namespace recs
{

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class QueryIterator;

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class Entity
{
  public:
    // Copy range and position to permit temporarily multiple entities from a
    // single Query
    // TODO:
    // Figure out how to do operator*/operator-> for Query
    Entity() = default;
    Entity(ComponentStorage::Range const *range, size_t pos);
    Entity(ComponentStorage::Range const *range);
    ~Entity() = default;

    Entity(Entity const &) = default;
    Entity &operator=(Entity const &) = default;

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
    ComponentStorage::Range const *m_range{nullptr};
    size_t m_pos{0};
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
    QueryIterator operator++(int);
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
    // This is not really an end iterator, but a dummy iterator that behaves
    // like one for operator== and operator!=. End iterators for two different
    // queries are equal.
    QueryIterator<ReadAccesses, WriteAccesses, WithAccesses> end() const
    {
        return QueryIterator<ReadAccesses, WriteAccesses, WithAccesses>();
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
    ComponentStorage::Range const *range, size_t pos)
: m_range{range}
, m_pos{pos}
{
    // TODO:
    // Figure out how to assert entity components here instead of the getters
    // for an earlier error. Getting types out of accesses seems tricky.
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
Entity<ReadAccesses, WriteAccesses, WithAccesses>::Entity(
    ComponentStorage::Range const *range)
: m_range{range}
{
    // TODO:
    // Figure out how to assert entity components here instead of the getters
    // for an earlier error. Getting types out of accesses seems tricky.
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
template <typename T>
    requires(Contains<ReadAccesses, T>)
T const &Entity<ReadAccesses, WriteAccesses, WithAccesses>::getComponent() const
{
    assert(m_range != nullptr);
    assert(
        m_range->hasComponent<T>(m_pos) &&
        "The entity is missing this component");
    return m_range->getComponent<T>(m_pos);
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
template <typename T>
    requires(Contains<WriteAccesses, T>)
T &Entity<ReadAccesses, WriteAccesses, WithAccesses>::getComponent() const
{
    assert(m_range != nullptr);
    assert(
        m_range->hasComponent<T>(m_pos) &&
        "The entity is missing this component");
    return m_range->getComponent<T>(m_pos);
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
QueryIterator<ReadAccesses, WriteAccesses, WithAccesses>::QueryIterator(
    ComponentStorage::Range const &range, size_t pos)
: m_range{&range}
, m_pos{pos}
, m_current_entity{m_range, m_pos}
{
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
QueryIterator<ReadAccesses, WriteAccesses, WithAccesses> &QueryIterator<
    ReadAccesses, WriteAccesses, WithAccesses>::operator++()
{
    assert(m_range != nullptr);
    assert(m_pos < m_range->size());
    m_pos++;
    m_current_entity = EntityType{m_range, m_pos};
    return *this;
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
QueryIterator<ReadAccesses, WriteAccesses, WithAccesses> QueryIterator<
    ReadAccesses, WriteAccesses, WithAccesses>::operator++(int)
{
    assert(m_range != nullptr);
    assert(m_pos < m_range->size());
    QueryIterator const ret = *this;
    m_pos++;
    m_current_entity = EntityType{m_range, m_pos};
    return ret;
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
    bool const this_is_end = m_range == nullptr || m_pos == m_range->size();
    bool const other_is_end =
        other.m_range == nullptr || other.m_pos == other.m_range->size();
    bool const ret = (this_is_end && other_is_end) ||
                     (m_range == other.m_range && m_pos == other.m_pos);
    return ret;
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
bool QueryIterator<ReadAccesses, WriteAccesses, WithAccesses>::operator!=(
    QueryIterator const &other) const
{
    bool const this_is_end = m_range == nullptr || m_pos == m_range->size();
    bool const other_is_end =
        other.m_range == nullptr || other.m_pos == other.m_range->size();
    bool const ret = (this_is_end != other_is_end) ||
                     (!this_is_end && other_is_end &&
                      (m_range != other.m_range || m_pos != other.m_pos));
    return ret;
}

} // namespace recs
