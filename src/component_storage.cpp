#include "recs/component_storage.hpp"

#include <atomic>
#include <bit>

namespace
{
size_t aligned_offset(size_t offset, size_t alignment)
{
    assert(std::has_single_bit(alignment));
    // TODO:
    // Get rid of this branch?
    if ((offset & (alignment - 1)) != 0)
        offset += alignment - (offset & (alignment - 1));
    return offset;
}

} // namespace

namespace recs
{

ComponentStorage::Range::Range(
    ComponentStorage const &cs,
    std::vector<EntitiesChunk::SubRange> &&subranges)
: m_cs{cs}
, m_subranges{std::move(subranges)}
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
    if (id.isEmpty())
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
            if (iter->first.test_all(mask))
                iters.emplace_back(iter);
        }
        m_mask_entitites.emplace(mask, iters);
    }

    std::vector<EntitiesChunk::SubRange> subranges;
    auto &entities = m_mask_entitites[mask];
    for (auto &cme : entities)
    {
        for (EntitiesChunk *ec : cme->second.m_chunks)
            std::copy(
                ec->m_subranges.begin(), ec->m_subranges.end(),
                std::back_inserter(subranges));
    }

    return Range{*this, std::move(subranges)};
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
    assert(!mask.empty());

    m_type_ids = mask.typeIds();

    size_t const componentCount = mask.count_ones();
    m_component_offsets = new size_t[componentCount];
    size_t offset = 0;
    size_t offset_i = 0;
    bool non_zero_sized_component_found = false;
    for (size_t id : m_type_ids)
    {
        // TODO:
        // Only go through components that are not empty tags?
        size_t const component_size = g_component_sizes[id];
        if (component_size == 0)
        {
            m_component_offsets[offset_i++] = offset;
            continue;
        }

        size_t const alignment = g_component_alignments[id];
        // We assume that we can pre-align offsets without knowing the
        // actual address we get
        assert(alignment <= s_base_alignment);
        offset = aligned_offset(offset, alignment);

        m_component_offsets[offset_i++] = offset;
        offset += component_size * s_max_entities;
        if (!non_zero_sized_component_found)
        {
            m_first_component_size = component_size;
            non_zero_sized_component_found = true;
        }
    }
    assert(non_zero_sized_component_found);

    size_t aligned_size = offset + s_base_alignment;
    m_data_unaligned = new uint8_t[aligned_size];

    void *data_ptr = m_data_unaligned;
    if (std::align(s_base_alignment, offset, data_ptr, aligned_size) != nullptr)
        m_data = static_cast<uint8_t *>(data_ptr);
    else
        assert(!"align failed");

    m_index_freelist.reserve(s_max_entities);
    static_assert(s_max_entities - 1 < 0xFFFF'FFFF);
    for (IndexT i = 0; i < static_cast<IndexT>(s_max_entities); ++i)
        m_index_freelist.push_back(static_cast<IndexT>(i));

    m_subranges.reserve((s_max_entities - 1) / 2 + 1);
}

EntitiesChunk::~EntitiesChunk()
{
    delete[] m_component_offsets;
    delete[] m_data_unaligned;
}

EntitiesChunk::EntitiesChunk(EntitiesChunk &&other) noexcept
: m_mask{other.m_mask}
, m_component_offsets{other.m_component_offsets}
, m_data_unaligned{other.m_data_unaligned}
, m_data{other.m_data}
, m_ids{other.m_ids}
, m_index_freelist{std::move(other.m_index_freelist)}
, m_type_ids{std::move(other.m_type_ids)}
{
    other.m_component_offsets = nullptr;
    other.m_data_unaligned = nullptr;
    other.m_data = nullptr;
}

EntitiesChunk &EntitiesChunk::operator=(EntitiesChunk &&other) noexcept
{
    if (this != &other)
    {
        m_mask = other.m_mask;
        m_component_offsets = other.m_component_offsets;
        m_data_unaligned = other.m_data_unaligned;
        m_data = other.m_data;
        m_ids = other.m_ids;
        m_index_freelist = std::move(other.m_index_freelist);
        m_type_ids = std::move(other.m_type_ids);

        other.m_component_offsets = nullptr;
        other.m_data_unaligned = nullptr;
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
    assert(component_size > 0 && "Can't query component data for empty types");

    uint8_t *ret = m_data + offset + entity_index * component_size;
    return ret;
}

bool ChunkEntityRef::isValid() const
{
    if (chunk == nullptr)
        return false;
    if (entity_index >= EntitiesChunk::s_max_entities)
        return false;
    if (chunk->m_ids[entity_index].isEmpty())
        return false;

    return true;
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

ComponentMaskEntities::~ComponentMaskEntities()
{
    for (EntitiesChunk *c : m_chunks)
        delete c;
    m_chunks.clear();
}

ChunkEntityRef ComponentMaskEntities::allocate(EntityId id)
{
    size_t chunk_index = 0;
    size_t const chunk_count = m_chunks.size();
    // TODO:
    // Should freelist be in this class instead of the individual chunks?
    for (; chunk_index < chunk_count; ++chunk_index)
    {
        EntitiesChunk const &chunk = *m_chunks[chunk_index];
        if (!chunk.m_index_freelist.empty())
            break;
    }

    if (chunk_index == chunk_count)
        m_chunks.emplace_back(new EntitiesChunk{m_mask});

    EntitiesChunk *chunk = m_chunks[chunk_index];
    EntitiesChunk::IndexT const entity_index = chunk->m_index_freelist.back();
    chunk->m_index_freelist.pop_back();
    assert(chunk->m_ids[entity_index].isEmpty());
    chunk->m_ids[entity_index] = id;

    size_t sr_index = 0;
    for (; sr_index < chunk->m_subranges.size(); ++sr_index)
    {
        EntitiesChunk::SubRange &sr = chunk->m_subranges[sr_index];
        if (entity_index < sr.first)
            break;
        sr_index++;
    }
    if (sr_index == chunk->m_subranges.size())
    {
        chunk->m_subranges.push_back(EntitiesChunk::SubRange{
            .first = entity_index,
            .last = entity_index,
            .chunk = chunk,
        });
    }
    else
    {
        EntitiesChunk::SubRange &sr = chunk->m_subranges[sr_index];
        if (sr_index == 0)
        {
            if (entity_index + 1 == sr.first)
                sr.first = entity_index;
            else
                chunk->m_subranges.insert(
                    chunk->m_subranges.begin(), EntitiesChunk::SubRange{
                                                    .first = entity_index,
                                                    .last = entity_index,
                                                    .chunk = chunk,
                                                });
        }
        else
        {
            assert(entity_index > 0);
            EntitiesChunk::SubRange &prev_sr = chunk->m_subranges[sr_index - 1];
            if (entity_index + 1 == sr.first)
            {
                if (entity_index - 1 == prev_sr.last)
                {
                    prev_sr.last = sr.last;
                    chunk->m_subranges.erase(
                        chunk->m_subranges.begin() + sr_index);
                }
                else
                    sr.first = entity_index;
            }
            else if (entity_index - 1 == prev_sr.last)
                prev_sr.last = entity_index;
            else
                chunk->m_subranges.insert(
                    chunk->m_subranges.begin(), EntitiesChunk::SubRange{
                                                    .first = entity_index,
                                                    .last = entity_index,
                                                    .chunk = chunk,
                                                });
        }
    }

    return ChunkEntityRef{
        .chunk = chunk,
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
        EntitiesChunk &chunk = *m_chunks[chunk_index];
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
#ifndef _NDEBUG
    for (size_t id : ref.chunk->m_type_ids)
    {
        void *ptr = ref.chunk->componentData(id, ref.entity_index);
        size_t const size = g_component_sizes[id];
        memset(ptr, 0xCD, size);
    }
#endif // !_NDEBUG
    ref.chunk->m_ids[ref.entity_index] = EntityId{};
    // Keep freelist sorted to guarantee allocate only pops indices at the end
    // of a hole
    // TODO:
    // This is expensive and gets prohibitively so if the freelist is moved to
    // ComponentMaskEntities as a list of ChunkRefs. Binary search will help
    // somewhat but it could still get really bad when a lot of entities are
    // free? What if a chunk has less entities and we just update all of the
    // tags in the holes we split when reallocating arbitrary indices?
    {
        auto it = ref.chunk->m_index_freelist.begin();
        for (; it != ref.chunk->m_index_freelist.end(); ++it)
        {
            if (*it > ref.entity_index)
                break;
            assert(*it != ref.entity_index);
        }
        ref.chunk->m_index_freelist.insert(it, ref.entity_index);
    }

    size_t sr_index = 0;
    for (; sr_index < ref.chunk->m_subranges.size(); ++sr_index)
    {
        EntitiesChunk::SubRange &sr = ref.chunk->m_subranges[sr_index];
        if (ref.entity_index >= sr.first && ref.entity_index <= sr.last)
            break;
    }
    assert(sr_index < ref.chunk->m_subranges.size());
    EntitiesChunk::SubRange &sr = ref.chunk->m_subranges[sr_index];
    assert(ref.entity_index >= sr.first && ref.entity_index <= sr.last);
    if (sr.first == sr.last)
        ref.chunk->m_subranges.erase(ref.chunk->m_subranges.begin() + sr_index);
    else if (ref.entity_index == sr.first)
        sr.first++;
    else if (ref.entity_index == sr.last)
        sr.last--;
    else
    {
        ref.chunk->m_subranges.insert(
            ref.chunk->m_subranges.begin() + sr_index,
            EntitiesChunk::SubRange{
                .first = sr.first,
                .last =
                    static_cast<EntitiesChunk::IndexT>(ref.entity_index - 1),
                .chunk = ref.chunk,
            });
        sr.first = ref.entity_index + 1;
    }
}

} // namespace recs
