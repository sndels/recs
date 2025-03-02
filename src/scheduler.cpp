#include "recs/scheduler.hpp"

namespace recs
{

SystemRef::SystemRef(Scheduler &scheduler, size_t index)
: m_scheduler{&scheduler}
, m_index{index}
{
}

SystemRef const &SystemRef::executeAfter(SystemRef dependency) const
{
    assert(m_scheduler != nullptr);
    assert(m_scheduler == dependency.m_scheduler);
    assert(m_index != dependency.m_index);
    assert(m_index < m_scheduler->m_systems.size());
    assert(dependency.m_index < m_scheduler->m_systems.size());

    Scheduler::System &this_sys = m_scheduler->m_systems[m_index];
    Scheduler::System &dep_sys = m_scheduler->m_systems[dependency.m_index];

    assert(!m_scheduler->dependsOn(dependency, *this) && "Cycle");

    this_sys.dependencies.push_back(dependency);
    dep_sys.dependents.push_back(*this);

    if (m_scheduler->m_roots.contains(m_index))
        m_scheduler->m_roots.erase(m_index);

    return *this;
}

bool SystemRef::operator==(SystemRef other) const
{
    return m_scheduler == other.m_scheduler && m_index == other.m_index;
}

bool SystemRef::operator!=(SystemRef other) const
{
    return m_scheduler != other.m_scheduler || m_index != other.m_index;
}

Schedule::Schedule(std::vector<SystemFunc> &&systems)
: m_systems{std::move(systems)}
{
}

void Schedule::execute(ComponentStorage &cs) const
{
    for (SystemFunc const &fn : m_systems)
        fn(cs);
}

Schedule Scheduler::buildSchedule()
{
    size_t const system_count = m_systems.size();
    assert(system_count > 0);

    // Do a simple topological sort from the roots
    std::vector<size_t> sorted_systems;
    sorted_systems.reserve(system_count);
    std::vector<size_t> traversal_stack;
    std::unordered_set<size_t> seen_systems;
    for (size_t root_i : m_roots)
    {
        assert(!seen_systems.contains(root_i));
        assert(m_systems[root_i].dependencies.empty());

        // Run DFS and push indices onto the (reverse) sorted vector on the way
        // out
        traversal_stack.clear();
        traversal_stack.push_back(root_i);
        while (!traversal_stack.empty())
        {
            size_t i = traversal_stack.back();
            if (seen_systems.contains(i))
            {
                sorted_systems.push_back(i);
                traversal_stack.pop_back();
                continue;
            }

            seen_systems.insert(i);
            System const &sys = m_systems[i];

            for (SystemRef dependent : sys.dependents)
            {
                // We check for cycles in executeAfter so assume this is not a
                // back-edge and the dependent is simply dependent on some
                // previously traversed root
                if (!seen_systems.contains(dependent.m_index))
                    traversal_stack.push_back(dependent.m_index);
            }
        }
    }

    std::vector<SystemFunc> systems;
    systems.reserve(system_count);
    // Our sorted list is in reverse execution order
    for (size_t i = system_count - 1; i > 0; --i)
    {
        size_t sys_i = sorted_systems[i];
        System const &sys = m_systems[sys_i];
        systems.push_back(sys.func);
    }
    size_t last_sys = sorted_systems[0];
    systems.push_back(m_systems[last_sys].func);

    Schedule s(std::move(systems));

    return s;
}

bool Scheduler::dependsOn(SystemRef dependent, SystemRef dependency) const
{
    assert(dependent.m_scheduler == this);
    assert(dependency.m_scheduler == this);
    assert(dependent != dependency);
    assert(dependent.m_index < m_systems.size());
    assert(dependency.m_index < m_systems.size());

    Scheduler::System const &dependent_sys = m_systems[dependent.m_index];

    for (SystemRef dep : dependent_sys.dependencies)
    {
        if (dep == dependency)
            return true;

        if (dependsOn(dep, dependency))
            return true;
    }

    return false;
}

} // namespace recs
