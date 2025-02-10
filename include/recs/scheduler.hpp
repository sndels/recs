#pragma once

#include "access.hpp"
#include "component_storage.hpp"
#include <functional>
#include <type_traits>
#include <unordered_set>

namespace recs
{

class Scheduler;

// Opaque hande so that this doesn't get invalidated when new systems are
// allocated
class SystemRef
{
  public:
    SystemRef() = default;
    ~SystemRef() = default;

    SystemRef(SystemRef const &other) = default;
    SystemRef &operator=(SystemRef const &other) = default;

    SystemRef const &executeAfter(SystemRef dependency) const;

    [[nodiscard]] bool operator==(SystemRef other) const;
    [[nodiscard]] bool operator!=(SystemRef other) const;

    friend class Scheduler;

  private:
    SystemRef(Scheduler &s, size_t index);

    Scheduler *m_scheduler{nullptr};
    size_t m_index{0};
};

using SystemFunc = std::function<void(ComponentStorage const &)>;

class Schedule
{
  public:
    ~Schedule() = default;

    Schedule(Schedule const &) = delete;
    Schedule(Schedule &&) = default;
    Schedule &operator=(Schedule const &) = delete;
    Schedule &operator=(Schedule &&) = default;

    void execute(ComponentStorage const &cs) const;

    friend class Scheduler;

  private:
    Schedule(std::vector<SystemFunc> &&systems);

    std::vector<SystemFunc> m_systems;
};

class Scheduler
{
  public:
    Scheduler() = default;
    ~Scheduler() = default;

    Scheduler(Scheduler const &) = delete;
    Scheduler(Scheduler &&) = default;
    Scheduler &operator=(Scheduler const &) = delete;
    Scheduler &operator=(Scheduler &&) = default;

    // TODO: Remedy had the system function as a template argument
    template <typename EntityReads, typename EntityWrites, typename EntityWiths>
    SystemRef registerSystem(
        void (*system)(Entity<EntityReads, EntityWrites, EntityWiths>));

    // TODO: Remedy had the system function as a template argument
    template <
        typename EntityReads, typename EntityWrites, typename EntityWiths,
        typename QueryReads, typename QueryWrites, typename QueryWiths>
    SystemRef registerSystem(void (*system)(
        Entity<EntityReads, EntityWrites, EntityWiths>,
        Query<QueryReads, QueryWrites, QueryWiths> const &));

    [[nodiscard]] Schedule buildSchedule();

    friend class SystemRef;

  private:
    struct System
    {
        SystemFunc func;
        std::vector<SystemRef> dependencies;
        std::vector<SystemRef> dependents;
    };

    [[nodiscard]] bool dependsOn(
        SystemRef dependent, SystemRef dependency) const;

    // Systems in this should not be reordered/removed after being added to keep
    // SystemRefs valid
    std::vector<System> m_systems;
    std::unordered_set<size_t> m_roots;
};

template <typename EntityReads, typename EntityWrites, typename EntityWiths>
SystemRef Scheduler::registerSystem(
    void (*system)(Entity<EntityReads, EntityWrites, EntityWiths>))
{
    using EntityT = Entity<EntityReads, EntityWrites, EntityWiths>;

    ComponentMask const access_mask = EntityT::accessMask();
    ComponentMask const write_access_mask = EntityT::writeAccessMask();

    System const s{
        .func =
            [system, access_mask](ComponentStorage const &cs)
        {
            Query<EntityReads, EntityWrites, EntityWiths> const entities_query{
                cs.getEntities(access_mask)};
            for (EntityT entity : entities_query)
                system(entity);
        },
    };

    SystemRef const ref{*this, m_systems.size()};

    m_systems.push_back(s);
    // No dependencies for a new system so mark as a root
    m_roots.insert(ref.m_index);

    return ref;
}

template <
    typename EntityReads, typename EntityWrites, typename EntityWiths,
    typename QueryReads, typename QueryWrites, typename QueryWiths>
SystemRef Scheduler::registerSystem(void (*system)(
    Entity<EntityReads, EntityWrites, EntityWiths>,
    Query<QueryReads, QueryWrites, QueryWiths> const &))
{
    using EntityT = Entity<EntityReads, EntityWrites, EntityWiths>;
    using QueryT = Query<QueryReads, QueryWrites, QueryWiths>;

    ComponentMask const access_mask = EntityT::accessMask();
    ComponentMask const write_access_mask = EntityT::writeAccessMask();
    ComponentMask const query_access_mask = QueryT::accessMask();
    ComponentMask const query_write_access_mask = QueryT::writeAccessMask();

    System const s{
        .func =
            [system, access_mask, query_access_mask](ComponentStorage const &cs)
        {
            QueryT const query{cs.getEntities(query_access_mask)};

            Query<EntityReads, EntityWrites, EntityWiths> const entities_query{
                cs.getEntities(access_mask)};
            for (EntityT entity : entities_query)
                system(entity, query);
        },
    };

    SystemRef const ref{*this, m_systems.size()};

    m_systems.push_back(s);
    // No dependencies for a new system so mark as a root
    m_roots.insert(ref.m_index);

    return ref;
}

} // namespace recs
