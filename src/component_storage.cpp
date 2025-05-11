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
            if (iter->first.test_all(mask))
                iters.emplace_back(iter);
        }
        m_mask_entitites.emplace(mask, iters);
    }

    std::vector<EntitiesChunk *> chunks;
    auto &entities = m_mask_entitites[mask];
    for (auto &cme : entities)
    {
        for (EntitiesChunk *ec : cme->second.m_chunks)
            chunks.push_back(ec);
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

    HoleTag *front_tag = holeTag(0);
    front_tag->skip_forward = static_cast<uint8_t>(s_max_entities);
    front_tag->skip_backward = 1;
    HoleTag *tail_tag = holeTag(s_max_entities - 1);
    tail_tag->skip_forward = 1;
    tail_tag->skip_backward = static_cast<uint8_t>(s_max_entities);

    m_index_freelist.reserve(s_max_entities);
    static_assert(s_max_entities - 1 < 0xFFFF'FFFF);
    for (IndexT i = 0; i < static_cast<IndexT>(s_max_entities); ++i)
        m_index_freelist.push_back(static_cast<IndexT>(i));
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

HoleTag *EntitiesChunk::holeTag(IndexT index) const
{
    assert(index < s_max_entities);
    size_t const offset = index * m_first_component_size;
    return reinterpret_cast<HoleTag *>(m_data + offset);
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

    EntitiesChunk &chunk = *m_chunks[chunk_index];
    EntitiesChunk::IndexT const entity_index = chunk.m_index_freelist.back();
    chunk.m_index_freelist.pop_back();
    assert(chunk.m_ids[entity_index] == EntityId{});
    chunk.m_ids[entity_index] = id;

    // Update hole tags
    HoleTag *tag = chunk.holeTag(entity_index);
    // We assume this is at the end of the hole
    assert(tag->skip_forward == 1);
    if (tag->skip_backward > 1)
    {
        assert(entity_index >= tag->skip_backward - 1);
        EntitiesChunk::IndexT const front_index =
            entity_index - (tag->skip_backward - 1);
        HoleTag *front_tag = chunk.holeTag(front_index);
        assert(front_tag->skip_backward == 1);
        front_tag->skip_forward--;

        HoleTag *tail_tag = front_tag;
        EntitiesChunk::IndexT tail_index = front_index;
        if (tag->skip_backward > 2)
        {
            tail_index = entity_index - 1;
            tail_tag = chunk.holeTag(tail_index);
            tail_tag->skip_forward = 1;
            tail_tag->skip_backward = tag->skip_backward - 1;
        }

        assert(front_tag->skip_forward == tail_tag->skip_backward);
        assert(front_tag->skip_forward == tail_index - front_index + 1);
    }

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

    // Update hole tags
    HoleTag *tag = ref.chunk->holeTag(ref.entity_index);
    tag->skip_forward = 1;
    tag->skip_backward = 1;

    HoleTag *front_tag = tag;
    EntitiesChunk::IndexT front_index = ref.entity_index;
    if (ref.entity_index > 0)
    {
        front_index = ref.entity_index - 1;
        EntityId prev_id = ref.chunk->m_ids[front_index];
        if (prev_id == EntityId{})
        {
            front_tag = ref.chunk->holeTag(front_index);
            assert(front_tag->skip_forward == 1);
            if (front_tag->skip_backward > 1)
            {
                assert(front_index < EntitiesChunk::s_max_entities);
                assert(front_index >= front_tag->skip_backward - 1);
                front_index = front_index - (front_tag->skip_backward - 1);
                front_tag = ref.chunk->holeTag(front_index);
                assert(front_tag->skip_backward == 1);
            }
        }
    }

    HoleTag *tail_tag = tag;
    EntitiesChunk::IndexT tail_index = ref.entity_index;
    if (ref.entity_index < EntitiesChunk::s_max_entities - 1)
    {
        EntitiesChunk::IndexT const next_index = ref.entity_index + 1;
        EntityId next_id = ref.chunk->m_ids[next_index];
        if (next_id == EntityId{})
        {
            tail_index = next_index;
            tail_tag = ref.chunk->holeTag(next_index);
            assert(tail_tag->skip_backward == 1);
            if (tail_tag->skip_forward > 1)
            {
                assert(tail_index < EntitiesChunk::s_max_entities);
                assert(
                    EntitiesChunk::s_max_entities - tail_index >=
                    tail_tag->skip_forward - 1);
                tail_index = tail_index + tail_tag->skip_forward - 1;
                tail_tag = ref.chunk->holeTag(tail_index);
                assert(tail_tag->skip_forward == 1);
            }
        }
    }

    if (front_tag != tail_tag)
    {
        uint8_t const jump_length = (tail_index - front_index) + 1;
        front_tag->skip_forward = jump_length;
        front_tag->skip_backward = 1;
        tail_tag->skip_forward = 1;
        tail_tag->skip_backward = jump_length;
    }
}

} // namespace recs
