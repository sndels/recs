#pragma once

#include <cassert>
#include <cstdint>

namespace recs
{

class ComponentStorage;

// NOTE:
// This assumes a single ComponentStorage as entities from multiple storages can
// be mixed up.
class EntityId
{
  public:
    EntityId() noexcept = default;
    ~EntityId() noexcept = default;

    EntityId(EntityId const &) noexcept = default;
    EntityId &operator=(EntityId const &) noexcept = default;

    [[nodiscard]] bool operator==(EntityId other) const noexcept
    {
        return m_gen_id == other.m_gen_id;
    }

    [[nodiscard]] bool operator!=(EntityId other) const noexcept
    {
        return m_gen_id != other.m_gen_id;
    }

    friend class ComponentStorage;

  private:
    static uint64_t const s_invalid_id = 0xFFFFFFFFFFFFFFFF;
    static uint8_t const s_index_bits = 48;
    static uint64_t const s_index_mask = 0xFFFFFFFFFFFF;
    static uint64_t const s_max_index = s_index_mask - 1;
    // 0xFFFF could be used mark handles that have been exhausted.
    static uint16_t const s_max_generation = 0xFFFE;

    EntityId(uint64_t index, uint16_t generation) noexcept
    : m_gen_id{(static_cast<uint64_t>(generation) << s_index_bits) | index}
    {
        assert(index <= s_max_index);
        assert(generation <= s_max_generation);
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        return m_gen_id != s_invalid_id;
    }

    [[nodiscard]] uint16_t generation() const noexcept
    {
        assert(m_gen_id != s_invalid_id);
        return m_gen_id >> s_index_bits;
    }

    [[nodiscard]] uint64_t index() const noexcept
    {
        assert(m_gen_id != s_invalid_id);
        return m_gen_id & s_index_mask;
    }

    uint64_t m_gen_id{s_invalid_id};
};

} // namespace recs
