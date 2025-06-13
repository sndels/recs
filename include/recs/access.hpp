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
    Entity() = default;
    Entity(ChunkEntityRef ref);
    ~Entity() = default;

    Entity(Entity const &) = default;
    Entity &operator=(Entity const &) = default;

    template <typename T>
        requires(Contains<ReadAccesses, T> && !std::is_empty_v<T>)
    [[nodiscard]] T const &getComponent() const;

    template <typename T>
        requires(Contains<WriteAccesses, T> && !std::is_empty_v<T>)
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

    friend class QueryIterator<ReadAccesses, WriteAccesses, WithAccesses>;

    // TODO:
    // Support structured bindings into components?
  protected:
    ChunkEntityRef m_chunk_ref;
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
    QueryIterator(QueryIterator &&) = delete;
    QueryIterator &operator=(QueryIterator const &) = default;
    QueryIterator &operator=(QueryIterator &&) = delete;

    QueryIterator &operator++();
    [[nodiscard]] EntityType &operator*();
    [[nodiscard]] EntityType const *operator->() const;
    [[nodiscard]] bool operator!=(QueryIterator const &other) const;
    [[nodiscard]] bool operator==(QueryIterator const &other) const;

    friend class Query<ReadAccesses, WriteAccesses, WithAccesses>;

  private:
    QueryIterator(
        ComponentStorage::Range const &range, size_t chunk_index,
        EntitiesChunk::IndexT entity_index);
    QueryIterator() = default;

    ComponentStorage::Range const &m_range;
    size_t m_chunk_index{(size_t)-1};
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
            m_range, 0, 0);
    }
    QueryIterator<ReadAccesses, WriteAccesses, WithAccesses> end() const
    {
        return QueryIterator<ReadAccesses, WriteAccesses, WithAccesses>(
            m_range, m_range.m_chunks.size(), 0);
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
Entity<ReadAccesses, WriteAccesses, WithAccesses>::Entity(ChunkEntityRef ref)
: m_chunk_ref{ref}
{
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
template <typename T>
    requires(Contains<ReadAccesses, T> && !std::is_empty_v<T>)
T const &Entity<ReadAccesses, WriteAccesses, WithAccesses>::getComponent() const
{
    assert(m_chunk_ref.isValid());
    return m_chunk_ref.chunk->getComponent<T>(m_chunk_ref.entity_index);
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
template <typename T>
    requires(Contains<WriteAccesses, T> && !std::is_empty_v<T>)
T &Entity<ReadAccesses, WriteAccesses, WithAccesses>::getComponent() const
{
    assert(m_chunk_ref.isValid());
    return m_chunk_ref.chunk->getComponent<T>(m_chunk_ref.entity_index);
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
QueryIterator<ReadAccesses, WriteAccesses, WithAccesses>::QueryIterator(
    ComponentStorage::Range const &range, size_t chunk_index,
    EntitiesChunk::IndexT entity_index)
: m_range{range}
, m_chunk_index{chunk_index}
{
    ChunkEntityRef &chunk_ref = m_current_entity.m_chunk_ref;
    chunk_ref.entity_index = entity_index;
    size_t const chunk_count = m_range.m_chunks.size();
    if (chunk_index >= chunk_count)
        return;
    chunk_ref.chunk = m_range.m_chunks[m_chunk_index];

    assert(chunk_ref.entity_index < EntitiesChunk::s_max_entities);
    while (m_chunk_index < chunk_count)
    {
        if (!chunk_ref.chunk->m_ids[chunk_ref.entity_index].isEmpty())
            break;
        HoleTag const *tag = chunk_ref.chunk->holeTag(chunk_ref.entity_index);
        assert(tag->skip_backward == 1);
        chunk_ref.entity_index += tag->skip_forward;
        if (chunk_ref.entity_index >= EntitiesChunk::s_max_entities)
        {
            chunk_ref.entity_index = 0;
            m_chunk_index++;
            if (m_chunk_index == chunk_count)
            {
                chunk_ref.chunk = nullptr;
                break;
            }
            chunk_ref.chunk = m_range.m_chunks[m_chunk_index];
        }
    }
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
QueryIterator<ReadAccesses, WriteAccesses, WithAccesses> &QueryIterator<
    ReadAccesses, WriteAccesses, WithAccesses>::operator++()
{
    size_t const chunk_count = m_range.m_chunks.size();
    if (m_chunk_index >= chunk_count)
        return *this;

    ChunkEntityRef &chunk_ref = m_current_entity.m_chunk_ref;
    chunk_ref.entity_index++;
    while (m_chunk_index < chunk_count)
    {
        if (chunk_ref.entity_index >= EntitiesChunk::s_max_entities)
        {
            chunk_ref.entity_index = 0;
            m_chunk_index++;
            if (m_chunk_index == chunk_count)
            {
                chunk_ref.chunk = nullptr;
                break;
            }
            chunk_ref.chunk = m_range.m_chunks[m_chunk_index];
        }
        if (!chunk_ref.chunk->m_ids[chunk_ref.entity_index].isEmpty())
            break;
        HoleTag const *tag = chunk_ref.chunk->holeTag(chunk_ref.entity_index);
        assert(tag->skip_backward == 1);
        chunk_ref.entity_index += tag->skip_forward;
    }
    return *this;
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
Entity<ReadAccesses, WriteAccesses, WithAccesses> &QueryIterator<
    ReadAccesses, WriteAccesses, WithAccesses>::operator*()
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
        &m_range == &other.m_range &&
        "Comparing iterators to different ranges");

    bool const ret = m_chunk_index == other.m_chunk_index &&
                     m_current_entity.m_chunk_ref.entity_index ==
                         other.m_current_entity.m_chunk_ref.entity_index;
    return ret;
}

template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
bool QueryIterator<ReadAccesses, WriteAccesses, WithAccesses>::operator!=(
    QueryIterator const &other) const
{
    assert(
        &m_range == &other.m_range &&
        "Comparing iterators to different ranges");

    bool const ret = m_chunk_index != other.m_chunk_index ||
                     m_current_entity.m_chunk_ref.entity_index !=
                         other.m_current_entity.m_chunk_ref.entity_index;
    return ret;
}

} // namespace recs
