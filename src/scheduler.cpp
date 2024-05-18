#include "recs/Scheduler.hpp"

namespace recs
{

void Scheduler::executeSystems(ComponentStorage const &cs) const
{
    for (System const &s : m_systems)
        s.func(cs);
}

} // namespace recs
