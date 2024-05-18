#pragma once

#include "access.hpp"
#include "component_storage.hpp"
#include <functional>
#include <type_traits>

namespace recs
{

class Scheduler
{
  public:
    Scheduler() = default;
    ~Scheduler() = default;

    Scheduler(Scheduler const &) = delete;
    Scheduler(Scheduler &&) = delete;
    Scheduler &operator=(Scheduler const &) = delete;
    Scheduler &operator=(Scheduler &&) = delete;

    // TODO: Remedy had the system function as a template argument
    template <typename EntityReads, typename EntityWrites, typename EntityWiths>
    void registerSystem(
        void (*system)(Entity<EntityReads, EntityWrites, EntityWiths>));

    // TODO: Remedy had the system function as a template argument
    template <
        typename EntityReads, typename EntityWrites, typename EntityWiths,
        typename QueryReads, typename QueryWrites, typename QueryWiths>
    void registerSystem(void (*system)(
        Entity<EntityReads, EntityWrites, EntityWiths>,
        Query<QueryReads, QueryWrites, QueryWiths> const &));

    void executeSystems(ComponentStorage const &cs) const;

  private:
    struct System
    {
        std::function<void(ComponentStorage const &)> func;
        ComponentMask access_mask;
        ComponentMask write_access_mask;
    };
    std::vector<System> m_systems;
};

template <typename EntityReads, typename EntityWrites, typename EntityWiths>
void Scheduler::registerSystem(
    void (*system)(Entity<EntityReads, EntityWrites, EntityWiths>))
{
    using EntityT = Entity<EntityReads, EntityWrites, EntityWiths>;

    ComponentMask const access_mask = EntityT::accessMask();
    ComponentMask const write_access_mask = EntityT::writeAccessMask();

    System const s{
        .func =
            [system, access_mask](ComponentStorage const &cs)
        {
            std::vector<EntityId> const ents = cs.getEntities(access_mask);
            for (EntityId const &id : ents)
                system(EntityT{cs, id});
        },
        .access_mask = access_mask,
        .write_access_mask = write_access_mask,
    };

    m_systems.push_back(s);
}

template <
    typename EntityReads, typename EntityWrites, typename EntityWiths,
    typename QueryReads, typename QueryWrites, typename QueryWiths>
void Scheduler::registerSystem(void (*system)(
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
            std::vector<EntityId> const query_ents =
                cs.getEntities(query_access_mask);
            QueryT const query{cs, query_ents};

            std::vector<EntityId> const ents = cs.getEntities(access_mask);
            for (EntityId const &id : ents)
                system(EntityT{cs, id}, query);
        },
        .access_mask = access_mask | query_access_mask,
        .write_access_mask = write_access_mask | query_write_access_mask,
    };

    m_systems.push_back(s);
}

} // namespace recs
