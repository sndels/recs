#pragma once

#include <cstddef>
#include <cstdint>
#include <new>

namespace recs
{

class TypeId
{
  public:
    // Use a reasonably large maximum to see the perf implications of this
    // design
    static size_t const s_max_component_type_count = 1024;

    // Returns a unique, thread-safe, constant id for the type. The ids can only
    // be depended on within the process they were queried in so they should not
    // be serialized.
    template <typename T> static uint64_t get();

  private:
    // Helper for typeId(), wrapping a thread-safe counter
    static uint64_t runningTypeId();
};

extern size_t g_component_sizes[TypeId::s_max_component_type_count];
extern std::align_val_t
    g_component_alignments[TypeId::s_max_component_type_count];

template <typename T> uint64_t TypeId::get()
{
    // Static init is required to be thread safe. runningTypeId is thread
    // safe in case multiple threads are initializing ids for different
    // component types.
    static uint64_t const id = []
    {
        uint64_t const _id = runningTypeId();
        g_component_sizes[_id] = sizeof(T);
        g_component_alignments[_id] = std::align_val_t{alignof(T)};
        return _id;
    }();
    return id;
}

} // namespace recs
