#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>

namespace recs
{

class TypeId
{
  public:
    // Use a reasonably large bitset to see the perf implications of this design
    static size_t const s_max_component_type_count = 1024;

    // Returns a unique, thread-safe, constant id for the type. The ids can only
    // be depended on within the process they were queried in so they should not
    // be serialized.
    template <typename T> static uint64_t get()
    {
        // Static init is required to be thread safe. runningTypeId is thread
        // safe in case multiple threads are initializing ids for different
        // component types.
        static uint64_t id = runningTypeId();
        return id;
    }

  private:
    // Helper for typeId(), wrapping a thread-safe counter
    static uint64_t runningTypeId();
};

// TODO: Should this be a in separate header?
using ComponentMask = std::bitset<TypeId::s_max_component_type_count>;

} // namespace recs
