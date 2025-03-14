#include "recs/naive_component_storage.hpp"

#include <atomic>

namespace recs::naive
{

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

} // namespace recs::naive
