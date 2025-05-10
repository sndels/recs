#pragma once

#include "access.hpp"
#include "component_storage.hpp"
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

struct SystemFunc
{
    void (*func_base)(SystemFunc const &, ComponentStorage &){nullptr};
    uintptr_t system_fn_ptr{0};

    template <typename EntityReads, typename EntityWrites, typename EntityWiths>
    static void funcBaseImplE(SystemFunc const &sys, ComponentStorage &cs)
    {
        using EntityT = Entity<EntityReads, EntityWrites, EntityWiths>;

        void (*system)(EntityT) =
            reinterpret_cast<void (*)(EntityT)>(sys.system_fn_ptr);

        ComponentMask const access_mask = EntityT::accessMask();
        Query<EntityReads, EntityWrites, EntityWiths> const entities_query{
            cs.getEntities(access_mask)};

        for (EntityT entity : entities_query)
            system(entity);
    }

    template <
        typename EntityReads, typename EntityWrites, typename EntityWiths,
        typename QueryReads, typename QueryWrites, typename QueryWiths>
    static void funcBaseImplEQ(SystemFunc const &sys, ComponentStorage &cs)
    {
        using EntityT = Entity<EntityReads, EntityWrites, EntityWiths>;
        using QueryT = Query<QueryReads, QueryWrites, QueryWiths>;

        void (*system)(EntityT, QueryT const &) =
            reinterpret_cast<void (*)(EntityT, QueryT const &)>(
                sys.system_fn_ptr);

        ComponentMask const query_access_mask = QueryT::accessMask();
        QueryT const query{cs.getEntities(query_access_mask)};

        ComponentMask const access_mask = EntityT::accessMask();
        Query<EntityReads, EntityWrites, EntityWiths> const entities_query{
            cs.getEntities(access_mask)};
        for (EntityT entity : entities_query)
            system(entity, query);
    }
};

class Schedule
{
  public:
    ~Schedule() = default;

    Schedule(Schedule const &) = delete;
    Schedule(Schedule &&) = default;
    Schedule &operator=(Schedule const &) = delete;
    Schedule &operator=(Schedule &&) = default;

    void execute(ComponentStorage &cs) const;

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

  protected:
    struct System
    {
        SystemFunc func;
        std::vector<SystemRef> dependencies;
        std::vector<SystemRef> dependents;
    };

  private:
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
    static_assert(sizeof(SystemFunc::system_fn_ptr) == sizeof(system));
    System const s{
        .func =
            SystemFunc{
                .func_base = &SystemFunc::funcBaseImplE<
                    EntityReads, EntityWrites, EntityWiths>,
                .system_fn_ptr = reinterpret_cast<uintptr_t>(system),
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
    static_assert(sizeof(SystemFunc::system_fn_ptr) == sizeof(system));
    System const s{
        .func =
            SystemFunc{
                .func_base = &SystemFunc::funcBaseImplEQ<
                    EntityReads, EntityWrites, EntityWiths, QueryReads,
                    QueryWrites, QueryWiths>,
                .system_fn_ptr = reinterpret_cast<uintptr_t>(system),
            },
    };

    SystemRef const ref{*this, m_systems.size()};

    m_systems.push_back(s);
    // No dependencies for a new system so mark as a root
    m_roots.insert(ref.m_index);

    return ref;
}

} // namespace recs
