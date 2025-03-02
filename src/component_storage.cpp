#include "recs/component_storage.hpp"

#include <atomic>

namespace recs
{

#ifdef RECS_HASHMAP_STORAGE

size_t ComponentStorage::Range::size() const { return m_entities.size(); }

bool ComponentStorage::Range::empty() const { return m_entities.empty(); }

EntityId ComponentStorage::Range::getId(size_t index) const
{
    assert(index < m_entities.size());
    return m_entities[index];
};

ComponentStorage::Range::Range(
    ComponentStorage const &cs, std::vector<EntityId> &&entities)
: m_cs{cs}
, m_entities{std::move(entities)}
{
}

ComponentStorage::~ComponentStorage()
{
    for (ComponentMap &cm : m_component_maps)
    {
        for (auto &[id, component] : cm)
            std::free(component);
    }
}

EntityId ComponentStorage::addEntity()
{
    uint64_t index;
    uint16_t generation;
    if (m_entity_freelist.empty())
    {
        assert(m_entity_generations.size() <= EntityId::s_max_index);
        index = (uint64_t)m_entity_generations.size();
        m_entity_generations.push_back(0);
        m_entity_alive.push_back(true);
        m_entity_component_masks.emplace_back();
        generation = 0;
    }
    else
    {
        // Pop from the front to avoid burning through generations on a single
        // handle when there are multiple free handles to choose from.
        index = m_entity_freelist.front();
        m_entity_freelist.pop_front();
        generation = m_entity_generations[index];
        // Freelist shouldn't have any handles that have exhausted their
        // generations
        assert(generation <= EntityId::s_max_generation);
        assert(!m_entity_alive[index]);
        m_entity_alive[index] = true;
    }

    EntityId const id{index, generation};
    return id;
}

bool ComponentStorage::isValid(EntityId id) const
{
    if (!id.isValid())
        return false;

    uint64_t const index = id.index();
    // Invalid index here is a bug because valid non-default initialized ids are
    // only handed constructed by this class
    assert(index < m_entity_generations.size());

    uint16_t const generation = id.generation();
    uint16_t const stored_generation = m_entity_generations[index];
    bool const generations_match = generation == stored_generation;

    assert(
        (!generations_match || m_entity_alive[index]) &&
        "Entity not marked alive is unexpected as stored generation matches "
        "the handle");

    return generations_match;
}

ComponentStorage::Range ComponentStorage::getEntities(
    ComponentMask const &mask) const
{
    std::vector<EntityId> ids;
    ids.reserve(m_entity_alive.size());

    uint64_t const entity_count = static_cast<uint64_t>(m_entity_alive.size());
    for (uint64_t id = 0; id < entity_count; ++id)
    {
        if (m_entity_alive[id])
        {
            ComponentMask const entityMask = m_entity_component_masks[id];
            if (entityMask.test_all(mask))
            {
                uint16_t const gen = m_entity_generations[id];
                ids.push_back(EntityId{id, gen});
            }
        }
    }

    ids.shrink_to_fit();

    return Range{*this, std::move(ids)};
}

void ComponentStorage::removeEntity(EntityId id)
{
    if (!isValid(id))
        return;

    uint64_t const index = id.index();
    uint16_t &stored_generation = m_entity_generations[index];
    stored_generation++;

    assert(m_entity_alive[index]);
    m_entity_alive[index] = false;

    ComponentMask &mask = m_entity_component_masks[index];
    for (size_t i = 0; i < ComponentMask::s_bit_count; ++i)
    {
        if (mask.test(i))
        {
            ComponentMap &cs = m_component_maps[i];

            void *ptr = cs.at(index);
            std::free(ptr);
            cs.erase(index);
        }
    }

    mask.reset();

    if (stored_generation <= EntityId::s_max_generation)
        m_entity_freelist.push_back(index);
}

#else // !RECS_HASHMAP_STORAGE

ComponentStorage::Range::Range(
    ComponentStorage const &cs, std::vector<EntitiesChunk *> &&chunks)
: m_cs{cs}
, m_chunks{std::move(chunks)}
{
}

EntityId ComponentStorage::addEntity()
{
    uint64_t index;
    uint16_t generation;
    if (m_entity_freelist.empty())
    {
        assert(m_entity_generations.size() <= EntityId::s_max_index);
        index = (uint64_t)m_entity_generations.size();
        m_entity_generations.push_back(0);
        m_entity_alive.push_back(true);
        m_entity_component_masks.emplace_back();
        m_entity_refs.emplace_back();
        generation = 0;
    }
    else
    {
        // Pop from the front to avoid burning through generations on a single
        // handle when there are multiple free handles to choose from.
        index = m_entity_freelist.front();
        m_entity_freelist.pop_front();
        generation = m_entity_generations[index];
        // Freelist shouldn't have any handles that have exhausted their
        // generations
        assert(generation <= EntityId::s_max_generation);
        assert(!m_entity_alive[index]);
        m_entity_alive[index] = true;
    }

    EntityId const id{index, generation};
    return id;
}

bool ComponentStorage::isValid(EntityId id) const
{
    if (!id.isValid())
        return false;

    uint64_t const index = id.index();
    // Invalid index here is a bug because valid non-default initialized ids are
    // only handed constructed by this class
    assert(index < m_entity_generations.size());

    uint16_t const generation = id.generation();
    uint16_t const stored_generation = m_entity_generations[index];
    bool const generations_match = generation == stored_generation;

    assert(
        (!generations_match || m_entity_alive[index]) &&
        "Entity not marked alive is unexpected as stored generation matches "
        "the handle");

    return generations_match;
}

ComponentStorage::Range ComponentStorage::getEntities(ComponentMask const &mask)
{
    if (!m_mask_entitites.contains(mask))
    {
        std::vector<decltype(m_storage)::iterator> iters;
        // New query, find all matching archetypes
        for (auto iter = m_storage.begin(); iter != m_storage.end(); ++iter)
        {
            if (iter->first.test_any(mask))
                iters.emplace_back(iter);
        }
        m_mask_entitites[mask] = iters;
    }

    std::vector<EntitiesChunk *> chunks;
    auto &entities = m_mask_entitites[mask];
    for (auto &cme : entities)
    {
        for (EntitiesChunk &ec : cme->second.m_chunks)
            chunks.push_back(&ec);
    }

    return Range{*this, std::move(chunks)};
}

ChunkEntityRef ComponentStorage::getEntity(EntityId id)
{
    assert(isValid(id));
    return m_entity_refs[id.index()];
}

void ComponentStorage::removeEntity(EntityId id)
{
    if (!isValid(id))
        return;

    uint64_t const index = id.index();
    uint16_t &stored_generation = m_entity_generations[index];
    stored_generation++;

    assert(m_entity_alive[index]);
    m_entity_alive[index] = false;

    ComponentMask &mask = m_entity_component_masks[index];

    if (!mask.empty())
    {
        assert(m_storage.contains(mask));
        ComponentMaskEntities &entities = m_storage.find(mask)->second;
        entities.destroy(id);
    }

    m_entity_refs[index].reset();
    mask.reset();

    if (stored_generation <= EntityId::s_max_generation)
        m_entity_freelist.push_back(index);
}

EntitiesChunk::EntitiesChunk(ComponentMask const &mask)
: m_mask{mask}
{
    size_t const componentCount = mask.count_ones();
    m_component_offsets = new size_t[componentCount];
    size_t offset = 0;
    size_t offset_i = 0;
    for (size_t i = 0; i < TypeId::s_max_component_type_count; ++i)
    {
        if (mask.test(i))
        {
            m_component_offsets[offset_i++] = offset;
            offset += g_component_sizes[i] * s_max_entities;
            offset += offset % sizeof(std::max_align_t);
        }
    }

    m_data = new uint8_t[offset];

    m_index_freelist.reserve(s_max_entities);
    static_assert(s_max_entities - 1 < 0xFFFF'FFFF);
    for (int32_t i = s_max_entities - 1; i >= 0; --i)
    {
        m_index_freelist.push_back(static_cast<IndexT>(i));
    }

    m_type_ids = mask.typeIds();
}

EntitiesChunk::~EntitiesChunk()
{
    delete[] m_component_offsets;
    delete[] m_data;
}

EntitiesChunk::EntitiesChunk(EntitiesChunk &&other) noexcept
: m_mask{other.m_mask}
, m_component_offsets{other.m_component_offsets}
, m_data{other.m_data}
, m_ids{other.m_ids}
, m_index_freelist{std::move(other.m_index_freelist)}
, m_type_ids{std::move(other.m_type_ids)}
{
    other.m_component_offsets = nullptr;
    other.m_data = nullptr;
}

EntitiesChunk &EntitiesChunk::operator=(EntitiesChunk &&other) noexcept
{
    if (this != &other)
    {
        m_mask = other.m_mask;
        m_component_offsets = other.m_component_offsets;
        m_data = other.m_data;
        m_ids = other.m_ids;
        m_index_freelist = std::move(other.m_index_freelist);
        m_type_ids = std::move(other.m_type_ids);

        other.m_component_offsets = nullptr;
        other.m_data = nullptr;
    }
    return *this;
}

EntitiesChunk::IndexT EntitiesChunk::index(EntityId id) const
{
    for (IndexT i = 0; i < s_max_entities; ++i)
    {
        if (m_ids[i] == id)
            return i;
    }
    return s_max_entities;
}

void *EntitiesChunk::componentData(
    uint64_t type_index, IndexT entity_index) const
{
    assert(m_mask.test(type_index));
    // TODO:
    // This offset query feels expensive to do for every entity. Is there a way
    // to cache it cheaper?
    size_t const component_index = m_mask.count_ones_left_of(type_index);
    size_t const offset = m_component_offsets[component_index];
    size_t const component_size = g_component_sizes[type_index];
    uint8_t *ret = m_data + offset + entity_index * component_size;
    return ret;
}

bool ChunkEntityRef::isValid() const
{
    return chunk != nullptr && entity_index < EntitiesChunk::s_max_entities;
}

void ChunkEntityRef::reset()
{
    chunk = nullptr;
    entity_index = EntitiesChunk::s_max_entities;
}

ComponentMaskEntities::ComponentMaskEntities(ComponentMask const &mask)
: m_mask{mask}
{
}

ChunkEntityRef ComponentMaskEntities::allocate(EntityId id)
{
    size_t chunk_index = 0;
    size_t const chunk_count = m_chunks.size();
    for (; chunk_index < chunk_count; ++chunk_index)
    {
        EntitiesChunk const &chunk = m_chunks[chunk_index];
        if (!chunk.m_index_freelist.empty())
            break;
    }

    if (chunk_index == chunk_count)
        m_chunks.emplace_back(m_mask);

    EntitiesChunk &chunk = m_chunks[chunk_index];
    EntitiesChunk::IndexT const entity_index = chunk.m_index_freelist.back();
    chunk.m_index_freelist.pop_back();
    assert(chunk.m_ids[entity_index] == EntityId{});
    chunk.m_ids[entity_index] = id;

    return ChunkEntityRef{
        .chunk = &chunk,
        .entity_index = entity_index,
    };
}

ChunkEntityRef ComponentMaskEntities::find(EntityId id)
{
    // TODO:
    // Hashmap entity->chunk to make this faster?
    size_t const chunk_count = m_chunks.size();
    for (size_t chunk_index = 0; chunk_index < chunk_count; ++chunk_index)
    {
        EntitiesChunk &chunk = m_chunks[chunk_index];
        EntitiesChunk::IndexT const entity_index = chunk.index(id);
        if (entity_index < EntitiesChunk::s_max_entities)
            return ChunkEntityRef{
                .chunk = &chunk,
                .entity_index = entity_index,
            };
    }
    return ChunkEntityRef{};
}

void ComponentMaskEntities::destroy(EntityId id)
{
    ChunkEntityRef const ref = find(id);
    assert(ref.isValid());
    // TODO:
    // memset component data to a pattern in debug?
    ref.chunk->m_ids[ref.entity_index] = EntityId{};
    ref.chunk->m_index_freelist.push_back(ref.entity_index);
}

#endif // RECS_HASHMAP_STORAGE

} // namespace recs
