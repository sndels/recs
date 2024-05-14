#include "recs/component_storage.hpp"

namespace recs
{

ComponentStorage::~ComponentStorage()
{
    for (auto &cm : m_component_maps)
    {
        for (auto &c : cm.second)
            std::free(c.second);
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
    return generation == stored_generation;
}

void ComponentStorage::removeEntity(EntityId id)
{
    if (!isValid(id))
        return;

    uint64_t const index = id.index();
    uint16_t &stored_generation = m_entity_generations[index];
    stored_generation++;

    // TODO:
    // There should probably be a bitmask or something so we don't have to probe
    // lots of component types the entity doesn't even have.
    for (auto &cs : m_component_maps)
    {
        if (cs.second.contains(index))
        {
            void *ptr = cs.second.at(index);
            std::free(ptr);
            cs.second.erase(index);
        }
    }

    if (stored_generation <= EntityId::s_max_generation)
        m_entity_freelist.push_back(index);
}

} // namespace recs
