#pragma once

#include "component_mask.hpp"
#include "concepts.hpp"
#include "entity_id.hpp"
#include "type_id.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace recs
{

// TODO: fwd.h
template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class Entity;
template <typename ReadAccesses, typename WriteAccesses, typename WithAccesses>
class QueryIterator;

// #define RECS_HASHMAP_STORAGE
#ifdef RECS_HASHMAP_STORAGE

class ComponentStorage
{
  public:
    // Don't bother restricting immutable component access because this is only
    // used directly by the strongly typed Query:Iterator
    struct Range
    {
        Range(ComponentStorage const &cs, std::vector<EntityId> &&entities);
        ~Range() = default;

        Range(Range &) = default;
        Range(Range &&) = default;
        Range &operator=(Range &) = delete;
        Range &operator=(Range &&) = delete;

        [[nodiscard]] size_t size() const;
        [[nodiscard]] bool empty() const;
        [[nodiscard]] EntityId getId(size_t index) const;
        template <typename T>
            requires ValidComponent<T>
        [[nodiscard]] T &getComponent(size_t index) const;

        ComponentStorage const &m_cs;
        std::vector<EntityId> m_entities;
    };

    ComponentStorage() = default;
    ~ComponentStorage();

    ComponentStorage(ComponentStorage const &) = delete;
    ComponentStorage(ComponentStorage const &&) = delete;
    ComponentStorage &operator=(ComponentStorage &) = delete;
    ComponentStorage &operator=(ComponentStorage &&) = delete;

    [[nodiscard]] EntityId addEntity();

    [[nodiscard]] bool isValid(EntityId id) const;

    [[nodiscard]] Range getEntities(ComponentMask const &mask) const;

    void removeEntity(EntityId id);

    template <typename T>
        requires ValidComponent<T>
    void addComponent(EntityId id, T const &c);

    template <typename T>
        requires ValidComponent<T>
    [[nodiscard]] bool hasComponent(EntityId id) const;
    template <typename... Ts>
        requires(ValidComponent<Ts> && ...)
    [[nodiscard]] bool hasComponents(EntityId id) const;

    template <typename T>
        requires ValidComponent<T>
    [[nodiscard]] T &getComponent(EntityId id) const;

    template <typename T>
        requires ValidComponent<T>
    void removeComponent(EntityId id);

  private:
    // TODO:
    // Not a linear array because content might be sparse. Still, this should be
    // some kind of sparse block allocated thing instead of a hashmap of
    // individual allocations per component instance to have some kind of cache
    // coherency and avoid hash lookups on every single entitys' every single
    // component. However, even if this was allocated as tight blocks, it would
    // still likely have terrible cache locality by itself: entities with
    // interesting subsets of components will be scattered around arbitrarily.
    // How should grouping be implemented?
    using ComponentMap = std::unordered_map<uint64_t, void *>;

    std::vector<ComponentMap> m_component_maps;
    std::vector<uint16_t> m_entity_generations;
    // TODO: This could be a bit in the stored generation
    std::vector<bool> m_entity_alive;
    std::deque<uint64_t> m_entity_freelist;

    std::vector<ComponentMask> m_entity_component_masks;
};

template <typename T>
    requires ValidComponent<T>
T &ComponentStorage::Range::getComponent(size_t index) const
{
    assert(index < m_entities.size());
    EntityId const id = m_entities[index];
    assert(m_cs.hasComponent<T>(id));
    return m_cs.getComponent<T>(id);
}

template <typename T>
    requires ValidComponent<T>
void ComponentStorage::addComponent(EntityId id, T const &c)
{
    assert(isValid(id));

    uint64_t const type_id = TypeId::get<T>();
    if (m_component_maps.size() <= type_id)
        m_component_maps.resize(type_id + 1);
    ComponentMap &map = m_component_maps[type_id];

    uint64_t const index = id.index();
    assert(!map.contains(index) && "The entity already has this component");

    T *ptr = (T *)std::malloc(sizeof(T));
    assert(ptr != nullptr);
    std::memcpy(ptr, &c, sizeof(T));

    map.emplace(index, ptr);

    assert(m_entity_component_masks.size() > index);
    ComponentMask &mask = m_entity_component_masks[index];
    assert(!mask.test(type_id));
    mask.set(type_id);
}

template <typename T>
    requires ValidComponent<T>
bool ComponentStorage::hasComponent(EntityId id) const
{
    assert(isValid(id));

    uint64_t const index = id.index();
    assert(m_entity_component_masks.size() > index);
    ComponentMask const &mask = m_entity_component_masks[index];

    uint64_t const type_id = TypeId::get<T>();
    return mask.test(type_id);
}

template <typename... Ts>
    requires(ValidComponent<Ts> && ...)
bool ComponentStorage::hasComponents(EntityId id) const
{
    return (hasComponent<Ts>(id) && ...);
}

template <typename T>
    requires ValidComponent<T>
T &ComponentStorage::getComponent(EntityId id) const
{
    assert(isValid(id));

    uint64_t const type_id = TypeId::get<T>();
    assert(type_id < m_component_maps.size());
    ComponentMap const &map = m_component_maps[type_id];

    uint64_t const index = id.index();
    T *ptr = (T *)map.at(index);
    assert(ptr != nullptr);

    return *ptr;
}

template <typename T>
    requires ValidComponent<T>
void ComponentStorage::removeComponent(EntityId id)
{
    assert(isValid(id));

    uint64_t const type_id = TypeId::get<T>();
    assert(type_id < m_component_maps.size());
    ComponentMap &map = m_component_maps[type_id];

    uint64_t const index = id.index();
    T *ptr = (T *)map.at(index);
    assert(ptr != nullptr);

    std::free(ptr);
    map.erase(index);

    assert(m_entity_component_masks.size() > index);
    ComponentMask &mask = m_entity_component_masks[index];
    assert(mask.test(type_id));
    mask.reset(type_id);
}

#else // !RECS_HASHMAP_STORAGE

struct EntitiesChunk;
struct ChunkEntityRef;
struct ComponentMaskEntities;

class ComponentStorage
{
  public:
    // Don't bother restricting immutable component access because this is only
    // used directly by the strongly typed Query:Iterator
    struct Range
    {
      public:
        ~Range() = default;

        Range(Range &) = default;
        Range(Range &&) = default;
        Range &operator=(Range &) = delete;
        Range &operator=(Range &&) = delete;

        Range(
            ComponentStorage const &cs, std::vector<EntitiesChunk *> &&chunks);

        ComponentStorage const &m_cs;
        std::vector<EntitiesChunk *> m_chunks;
    };

    ComponentStorage() = default;
    ~ComponentStorage() = default;

    ComponentStorage(ComponentStorage const &) = delete;
    ComponentStorage(ComponentStorage const &&) = delete;
    ComponentStorage &operator=(ComponentStorage &) = delete;
    ComponentStorage &operator=(ComponentStorage &&) = delete;

    [[nodiscard]] EntityId addEntity();

    [[nodiscard]] bool isValid(EntityId id) const;

    [[nodiscard]] Range getEntities(ComponentMask const &mask);
    [[nodiscard]] ChunkEntityRef getEntity(EntityId id);

    void removeEntity(EntityId id);

    template <typename T>
        requires ValidComponent<T>
    void addComponent(EntityId id, T const &c);

    template <typename T>
        requires ValidComponent<T>
    [[nodiscard]] bool hasComponent(EntityId id) const;
    template <typename... Ts>
        requires(ValidComponent<Ts> && ...)
    [[nodiscard]] bool hasComponents(EntityId id) const;

    template <typename T>
        requires ValidComponent<T>
    [[nodiscard]] T &getComponent(EntityId id);

    template <typename T>
        requires ValidComponent<T>
    void removeComponent(EntityId id);

  private:
    std::unordered_map<ComponentMask, ComponentMaskEntities> m_storage;
    std::unordered_map<
        ComponentMask, std::vector<decltype(m_storage)::iterator>>
        m_mask_entitites;

    std::vector<uint16_t> m_entity_generations;
    // TODO: This could be a bit in the stored generation
    std::vector<bool> m_entity_alive;
    std::deque<uint64_t> m_entity_freelist;
    std::vector<ComponentMask> m_entity_component_masks;
    std::vector<ChunkEntityRef> m_entity_refs;
};

struct EntitiesChunk
{
    static constexpr size_t s_max_entities = 128;
    using IndexT = uint8_t;
    static_assert(s_max_entities <= static_cast<IndexT>(0xFFFF'FFFF'FFFF'FFFF));

    ComponentMask m_mask;
    size_t *m_component_offsets;
    // Storage for all components that are in m_mask. Each component type is
    // stored in a separate block, ordered according to the component bits.
    uint8_t *m_data;
    std::array<EntityId, s_max_entities> m_ids;
    std::vector<IndexT> m_index_freelist;
    std::vector<uint64_t> m_type_ids;

    EntitiesChunk(ComponentMask const &mask);
    ~EntitiesChunk();

    EntitiesChunk(EntitiesChunk const &) = delete;
    EntitiesChunk(EntitiesChunk &&other) noexcept;
    EntitiesChunk &operator=(EntitiesChunk const &) = delete;
    EntitiesChunk &operator=(EntitiesChunk &&other) noexcept;

    [[nodiscard]] IndexT index(EntityId id) const;

    template <typename T>
        requires ValidComponent<T>
    [[nodiscard]] T &getComponent(IndexT index);
    void *componentData(uint64_t type_index, IndexT entity_index) const;
};

struct ChunkEntityRef
{
    EntitiesChunk *chunk{nullptr};
    EntitiesChunk::IndexT entity_index{EntitiesChunk::s_max_entities};

    [[nodiscard]] bool isValid() const;
    void reset();
};

struct ComponentMaskEntities
{
    ComponentMaskEntities(ComponentMask const &mask);
    ~ComponentMaskEntities() = default;

    ComponentMaskEntities(ComponentMaskEntities const &) = delete;
    ComponentMaskEntities(ComponentMaskEntities &&) = delete;
    ComponentMaskEntities &operator=(ComponentMaskEntities const &) = delete;
    ComponentMaskEntities &operator=(ComponentMaskEntities &&) = delete;

    [[nodiscard]] ChunkEntityRef allocate(EntityId id);
    [[nodiscard]] ChunkEntityRef find(EntityId id);
    void destroy(EntityId id);

    template <typename T>
        requires ValidComponent<T>
    [[nodiscard]] T &getComponent(EntityId id) const;

    ComponentMask m_mask;
    std::vector<EntitiesChunk> m_chunks;
};

template <typename T>
    requires ValidComponent<T>
T &EntitiesChunk::getComponent(IndexT entity_index)
{
    T *ret =
        reinterpret_cast<T *>(componentData(TypeId::get<T>(), entity_index));
    return *ret;
}

template <typename T>
    requires ValidComponent<T>
[[nodiscard]] T &ComponentMaskEntities::getComponent(EntityId id) const
{
    // TODO:
    // Hashmap entity->chunk to make this faster?
    size_t const chunk_count = m_chunks.size();
    for (size_t chunk_index = 0; chunk_index < chunk_count; ++chunk_index)
    {
        EntitiesChunk const &chunk = m_chunks[chunk_index];
        EntitiesChunk::IndexT const entity_index = chunk.index(id);
        if (entity_index < EntitiesChunk::s_max_entities)
            return chunk.getComponent<T>(entity_index);
    }
    // TODO: Return pointers, null instead of having exceptions?
    throw std::runtime_error("Component not found");
}

template <typename T>
    requires ValidComponent<T>
void ComponentStorage::addComponent(EntityId id, T const &c)
{
    assert(isValid(id));
    uint64_t const entity_index = id.index();

    ComponentMask old_mask = m_entity_component_masks[entity_index];
    assert(!old_mask.test<T>() && "The entity already has this component");

    ComponentMask new_mask = old_mask;
    new_mask.set<T>();
    m_entity_component_masks[entity_index] = new_mask;

    if (!m_storage.contains(new_mask))
    {
        auto iter = m_storage.try_emplace(new_mask, new_mask).first;
        // Fill already used submasks with the new one
        for (auto &[other_mask, iters] : m_mask_entitites)
        {
            if (other_mask.test_any(new_mask))
                iters.emplace_back(iter);
        }
        m_mask_entitites.emplace(new_mask, std::vector{iter});
    }

    ComponentMaskEntities &new_storage = m_storage.at(new_mask);
    ChunkEntityRef const new_allocation = new_storage.allocate(id);
    if (!old_mask.empty())
    {
        assert(m_storage.contains(old_mask));

        ComponentMaskEntities &old_storage = m_storage.at(old_mask);
        ChunkEntityRef const old_allocation = old_storage.find(id);

        for (uint64_t const type_index : old_allocation.chunk->m_type_ids)
        {
            void *old_data = old_allocation.chunk->componentData(
                type_index, old_allocation.entity_index);
            void *new_data = new_allocation.chunk->componentData(
                type_index, new_allocation.entity_index);
            size_t const data_size = g_component_sizes[type_index];
            memcpy(new_data, old_data, data_size);
        }
        old_storage.destroy(id);
    }
    new_allocation.chunk->getComponent<T>(new_allocation.entity_index) = c;

    m_entity_refs[entity_index] = new_allocation;
}

template <typename T>
    requires ValidComponent<T>
bool ComponentStorage::hasComponent(EntityId id) const
{
    assert(isValid(id));

    uint64_t const index = id.index();
    assert(m_entity_component_masks.size() > index);
    ComponentMask const &mask = m_entity_component_masks[index];

    uint64_t const type_id = TypeId::get<T>();
    return mask.test(type_id);
}

template <typename... Ts>
    requires(ValidComponent<Ts> && ...)
bool ComponentStorage::hasComponents(EntityId id) const
{
    return (hasComponent<Ts>(id) && ...);
}

template <typename T>
    requires ValidComponent<T>
T &ComponentStorage::getComponent(EntityId id)
{
    assert(isValid(id));
    uint64_t const index = id.index();

    ComponentMask const &mask = m_entity_component_masks[index];
    assert(mask.test(TypeId::get<T>()));

    ComponentMaskEntities &storage = m_storage.find(mask)->second;
    ChunkEntityRef const allocation = storage.find(id);

    return allocation.chunk->getComponent<T>(allocation.entity_index);
}

template <typename T>
    requires ValidComponent<T>
void ComponentStorage::removeComponent(EntityId id)
{
    assert(isValid(id));
    uint64_t const entity_index = id.index();

    ComponentMask old_mask = m_entity_component_masks[entity_index];
    assert(old_mask.test<T>() && "The entity doesn't have this component");

    ComponentMask new_mask = old_mask;
    new_mask.reset<T>();
    m_entity_component_masks[entity_index] = new_mask;

    if (!m_storage.contains(new_mask))
    {
        auto iter = m_storage.try_emplace(new_mask, new_mask).first;
        // Fill already used submasks with the new one
        for (auto &[other_mask, iters] : m_mask_entitites)
        {
            if (other_mask.test_any(new_mask))
                iters.emplace_back(iter);
        }
        m_mask_entitites.emplace(new_mask, std::vector{iter});
    }

    ComponentMaskEntities &old_storage = m_storage.at(old_mask);
    ChunkEntityRef const old_allocation = old_storage.find(id);
    if (new_mask.empty())
        m_entity_refs[entity_index].reset();
    else
    {
        assert(m_storage.contains(old_mask));
        ComponentMaskEntities &new_storage = m_storage.at(new_mask);
        ChunkEntityRef const new_allocation = new_storage.allocate(id);

        for (uint64_t const type_index : old_allocation.chunk->m_type_ids)
        {
            if (type_index == TypeId::get<T>())
                continue;
            void *old_data = old_allocation.chunk->componentData(
                type_index, old_allocation.entity_index);
            void *new_data = new_allocation.chunk->componentData(
                type_index, new_allocation.entity_index);
            size_t const data_size = g_component_sizes[type_index];
            memcpy(new_data, old_data, data_size);
        }
    }

    old_storage.destroy(id);
}

#endif // RECS_HASHMAP_STORAGE

} // namespace recs
