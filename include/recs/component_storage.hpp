#pragma once

#include "concepts.hpp"
#include "entity_id.hpp"
#include "type_id.hpp"
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace recs
{

class ComponentStorage
{
  public:
    ComponentStorage() = default;
    ~ComponentStorage();

    ComponentStorage(ComponentStorage const &) = delete;
    ComponentStorage(ComponentStorage const &&) = delete;
    ComponentStorage &operator=(ComponentStorage &) = delete;
    ComponentStorage &operator=(ComponentStorage &&) = delete;

    [[nodiscard]] EntityId addEntity();

    [[nodiscard]] bool isValid(EntityId id) const;

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
    std::deque<uint64_t> m_entity_freelist;

    std::vector<ComponentMask> m_entity_component_masks;
};

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
    requires ValidComponent<T> bool
ComponentStorage::hasComponent(EntityId id) const
{
    assert(isValid(id));

    uint64_t const index = id.index();
    assert(m_entity_component_masks.size() > index);
    ComponentMask const &mask = m_entity_component_masks[index];

    uint64_t const type_id = TypeId::get<T>();
    return mask.test(type_id);
}

template <typename... Ts>
    requires(ValidComponent<Ts> && ...) bool
ComponentStorage::hasComponents(EntityId id) const
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

} // namespace recs
