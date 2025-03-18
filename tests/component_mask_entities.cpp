#include <catch2/catch_test_macros.hpp>

#include "recs/component_storage.hpp"

namespace
{

bool all_unique_indices(std::vector<recs::ChunkEntityRef> const &v)
{
    for (size_t i = 0; i < v.size(); ++i)
    {
        for (size_t j = i + 1; j < v.size(); ++j)
        {
            if (v[i].entity_index == v[j].entity_index)
                return false;
        }
    }
    return true;
}

bool all_same_chunk(std::vector<recs::ChunkEntityRef> const &v)
{
    assert(!v.empty());
    recs::EntitiesChunk *first_chunk = v.front().chunk;

    for (size_t i = 1; i < v.size(); ++i)
    {
        if (v[i].chunk != first_chunk)
            return false;
    }
    return true;
}

} // namespace

namespace recs::tests
{

struct EntityIdCreator
{
    static EntityId create(uint64_t index, uint16_t generation)
    {
        return {index, generation};
    }
};

} // namespace recs::tests

TEST_CASE("ComponentMaskEntities")
{
    recs::ComponentMask single_mask;
    single_mask.set<int16_t>();
    recs::ComponentMaskEntities single_storage(single_mask);

    recs::ComponentMask multiple_mask;
    multiple_mask.set<int64_t>();
    multiple_mask.set<int16_t>();
    recs::ComponentMaskEntities multiple_storage(multiple_mask);

    std::vector<recs::EntityId> ids;
    std::vector<recs::ChunkEntityRef> single_mask_chunks;
    std::vector<recs::ChunkEntityRef> multiple_mask_chunks;

    // Allocate full chunk
    for (size_t id = 0; id < recs::EntitiesChunk::s_max_entities; ++id)
    {
        recs::EntityId const eid = recs::tests::EntityIdCreator::create(id, 0);
        ids.push_back(eid);
        single_mask_chunks.push_back(single_storage.allocate(eid));
        multiple_mask_chunks.push_back(multiple_storage.allocate(eid));
    }
    REQUIRE(all_unique_indices(single_mask_chunks));
    REQUIRE(all_same_chunk(single_mask_chunks));
    REQUIRE(single_mask_chunks[0].chunk->m_index_freelist.empty());
    REQUIRE(all_unique_indices(multiple_mask_chunks));
    REQUIRE(all_same_chunk(multiple_mask_chunks));
    REQUIRE(multiple_mask_chunks[0].chunk->m_index_freelist.empty());

    // Destroy every other slot
    for (size_t i = 1; i < ids.size(); ++i)
    {
        single_storage.destroy(ids[i]);
        multiple_storage.destroy(ids[i]);
        ids.erase(ids.begin() + i);
        single_mask_chunks.erase(single_mask_chunks.begin() + i);
        multiple_mask_chunks.erase(multiple_mask_chunks.begin() + i);
    }
    // Reallocate back to full
    while (ids.size() < recs::EntitiesChunk::s_max_entities)
    {
        // Let's be sure to not pass in entity ids that clash with existing ones
        recs::EntityId const eid = recs::tests::EntityIdCreator::create(
            recs::EntitiesChunk::s_max_entities + ids.size() - 1, 0);
        ids.push_back(eid);
        single_mask_chunks.push_back(single_storage.allocate(eid));
        multiple_mask_chunks.push_back(multiple_storage.allocate(eid));
    }
    REQUIRE(all_unique_indices(single_mask_chunks));
    REQUIRE(all_same_chunk(single_mask_chunks));
    REQUIRE(single_mask_chunks[0].chunk->m_index_freelist.empty());
    REQUIRE(all_unique_indices(multiple_mask_chunks));
    REQUIRE(all_same_chunk(multiple_mask_chunks));
    REQUIRE(multiple_mask_chunks[0].chunk->m_index_freelist.empty());

    // Destroy front window
    for (size_t i : {0, 1, 2, 3, 4, 5})
    {
        single_storage.destroy(ids[i]);
        multiple_storage.destroy(ids[i]);
        ids.erase(ids.begin() + i);
        single_mask_chunks.erase(single_mask_chunks.begin() + i);
        multiple_mask_chunks.erase(multiple_mask_chunks.begin() + i);
    }
    // Reallocate back to full
    while (ids.size() < recs::EntitiesChunk::s_max_entities)
    {
        // Let's be sure to not pass in entity ids that clash with existing ones
        recs::EntityId const eid = recs::tests::EntityIdCreator::create(
            recs::EntitiesChunk::s_max_entities + ids.size() - 1, 0);
        ids.push_back(eid);
        single_mask_chunks.push_back(single_storage.allocate(eid));
        multiple_mask_chunks.push_back(multiple_storage.allocate(eid));
    }
    REQUIRE(all_unique_indices(single_mask_chunks));
    REQUIRE(all_same_chunk(single_mask_chunks));
    REQUIRE(single_mask_chunks[0].chunk->m_index_freelist.empty());
    REQUIRE(all_unique_indices(multiple_mask_chunks));
    REQUIRE(all_same_chunk(multiple_mask_chunks));
    REQUIRE(multiple_mask_chunks[0].chunk->m_index_freelist.empty());

    // Destroy back window
    for (size_t i = recs::EntitiesChunk::s_max_entities - 1;
         i < recs::EntitiesChunk::s_max_entities - 6; --i)
    {
        single_storage.destroy(ids[i]);
        multiple_storage.destroy(ids[i]);
        ids.erase(ids.begin() + i);
        single_mask_chunks.erase(single_mask_chunks.begin() + i);
        multiple_mask_chunks.erase(multiple_mask_chunks.begin() + i);
    }
    // Reallocate back to full
    while (ids.size() < recs::EntitiesChunk::s_max_entities)
    {
        // Let's be sure to not pass in entity ids that clash with existing ones
        recs::EntityId const eid = recs::tests::EntityIdCreator::create(
            recs::EntitiesChunk::s_max_entities + ids.size() - 1, 0);
        ids.push_back(eid);
        single_mask_chunks.push_back(single_storage.allocate(eid));
        multiple_mask_chunks.push_back(multiple_storage.allocate(eid));
    }
    REQUIRE(all_unique_indices(single_mask_chunks));
    REQUIRE(all_same_chunk(single_mask_chunks));
    REQUIRE(single_mask_chunks[0].chunk->m_index_freelist.empty());
    REQUIRE(all_unique_indices(multiple_mask_chunks));
    REQUIRE(all_same_chunk(multiple_mask_chunks));
    REQUIRE(multiple_mask_chunks[0].chunk->m_index_freelist.empty());

    // Destroy middle window
    for (size_t i : {10, 11, 12, 13, 14})
    {
        single_storage.destroy(ids[i]);
        multiple_storage.destroy(ids[i]);
        ids.erase(ids.begin() + i);
        single_mask_chunks.erase(single_mask_chunks.begin() + i);
        multiple_mask_chunks.erase(multiple_mask_chunks.begin() + i);
    }
    // Reallocate back to full
    while (ids.size() < recs::EntitiesChunk::s_max_entities)
    {
        // Let's be sure to not pass in entity ids that clash with existing ones
        recs::EntityId const eid = recs::tests::EntityIdCreator::create(
            recs::EntitiesChunk::s_max_entities + ids.size() - 1, 0);
        ids.push_back(eid);
        single_mask_chunks.push_back(single_storage.allocate(eid));
        multiple_mask_chunks.push_back(multiple_storage.allocate(eid));
    }
    REQUIRE(all_unique_indices(single_mask_chunks));
    REQUIRE(all_same_chunk(single_mask_chunks));
    REQUIRE(single_mask_chunks[0].chunk->m_index_freelist.empty());
    REQUIRE(all_unique_indices(multiple_mask_chunks));
    REQUIRE(all_same_chunk(multiple_mask_chunks));
    REQUIRE(multiple_mask_chunks[0].chunk->m_index_freelist.empty());

    // Destroy middle window random order
    for (size_t i : {12, 10, 14, 11, 13})
    {
        single_storage.destroy(ids[i]);
        multiple_storage.destroy(ids[i]);
        ids.erase(ids.begin() + i);
        single_mask_chunks.erase(single_mask_chunks.begin() + i);
        multiple_mask_chunks.erase(multiple_mask_chunks.begin() + i);
    }
    // Reallocate back to full
    while (ids.size() < recs::EntitiesChunk::s_max_entities)
    {
        // Let's be sure to not pass in entity ids that clash with existing ones
        recs::EntityId const eid = recs::tests::EntityIdCreator::create(
            recs::EntitiesChunk::s_max_entities + ids.size() - 1, 0);
        ids.push_back(eid);
        single_mask_chunks.push_back(single_storage.allocate(eid));
        multiple_mask_chunks.push_back(multiple_storage.allocate(eid));
    }
    REQUIRE(all_unique_indices(single_mask_chunks));
    REQUIRE(all_same_chunk(single_mask_chunks));
    REQUIRE(single_mask_chunks[0].chunk->m_index_freelist.empty());
    REQUIRE(all_unique_indices(multiple_mask_chunks));
    REQUIRE(all_same_chunk(multiple_mask_chunks));
    REQUIRE(multiple_mask_chunks[0].chunk->m_index_freelist.empty());
}
