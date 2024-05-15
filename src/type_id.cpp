#include "recs/type_id.hpp"

#include <atomic>
#include <cassert>

namespace recs
{

uint64_t TypeId::runningTypeId()
{
    // TODO:
    // This should be ok even if used in a DLL but potentially not if the DLL is
    // shared between processes?
    static std::atomic<uint64_t> id = 0;
    // Static init is thread safe but multiple threads might be initializing
    // type ids for different types.
    uint64_t ret = id.fetch_add(1);
    assert(ret <= s_max_component_type_count);
    return ret;
}

} // namespace recs
