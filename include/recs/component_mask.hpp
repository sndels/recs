#pragma once

#include "concepts.hpp"
#include "inline.hpp"
#include "type_id.hpp"
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstring>
// For std::hash
#include <memory>
#include <vector>

namespace recs
{

// TODO: Should this be a in separate header?
class ComponentMask
{
  public:
    static constexpr uint64_t s_bit_count = TypeId::s_max_component_type_count;

    ComponentMask() = default;
    ~ComponentMask() = default;

    ComponentMask(ComponentMask const &) = default;
    ComponentMask(ComponentMask &&) = default;
    ComponentMask &operator=(ComponentMask const &) = default;
    ComponentMask &operator=(ComponentMask &&) = default;

    [[nodiscard]] RECS_FORCEINLINE ComponentMask
    operator&(ComponentMask const &other) const;
    [[nodiscard]] RECS_FORCEINLINE bool operator==(
        ComponentMask const &other) const;
    [[nodiscard]] RECS_FORCEINLINE bool operator!=(
        ComponentMask const &other) const;

    [[nodiscard]] RECS_FORCEINLINE std::vector<uint64_t> typeIds() const;

    template <typename ComponentT>
        requires ValidComponent<ComponentT>
    RECS_FORCEINLINE void set();
    template <typename ComponentT>
        requires ValidComponent<ComponentT>
    RECS_FORCEINLINE void reset();
    template <typename ComponentT>
        requires ValidComponent<ComponentT>
    RECS_FORCEINLINE bool test();
    template <typename ComponentT>
        requires ValidComponent<ComponentT>
    [[nodiscard]] RECS_FORCEINLINE uint64_t count_ones_left_of() const;

    RECS_FORCEINLINE void set();
    RECS_FORCEINLINE void set(uint64_t pos);
    RECS_FORCEINLINE void reset();
    RECS_FORCEINLINE void reset(uint64_t pos);
    [[nodiscard]] RECS_FORCEINLINE bool test(uint64_t pos) const;
    [[nodiscard]] RECS_FORCEINLINE bool test_all(
        ComponentMask const &other) const;
    [[nodiscard]] RECS_FORCEINLINE bool test_any(
        ComponentMask const &other) const;
    [[nodiscard]] RECS_FORCEINLINE bool empty() const;

    [[nodiscard]] RECS_FORCEINLINE uint64_t count_ones() const;
    [[nodiscard]] RECS_FORCEINLINE uint64_t
    count_ones_left_of(uint64_t pos) const;
    [[nodiscard]] RECS_FORCEINLINE uint64_t count_zeros() const;
    [[nodiscard]] RECS_FORCEINLINE uint64_t count_leading_zeros() const;
    [[nodiscard]] RECS_FORCEINLINE uint64_t count_leading_ones() const;
    [[nodiscard]] RECS_FORCEINLINE uint64_t count_trailing_zeros() const;
    [[nodiscard]] RECS_FORCEINLINE uint64_t count_trailing_ones() const;

    [[nodiscard]] RECS_FORCEINLINE uint64_t hash() const;

  private:
    using block_t = uint64_t;
    static_assert(s_bit_count % (sizeof(block_t) * 8) == 0);
    static constexpr uint64_t s_block_bit_count = sizeof(block_t) * 8;
    static constexpr uint64_t s_block_count = s_bit_count / s_block_bit_count;

    static RECS_FORCEINLINE uint64_t s_block_index(uint64_t pos);
    static RECS_FORCEINLINE uint64_t s_bit_index(uint64_t pos);

    block_t m_blocks[s_block_count]{};
};

RECS_FORCEINLINE ComponentMask
ComponentMask::operator&(ComponentMask const &other) const
{
    ComponentMask ret;
    for (uint64_t i = 0; i < s_block_count; ++i)
        ret.m_blocks[i] = m_blocks[i] & other.m_blocks[i];
    return ret;
}

RECS_FORCEINLINE bool ComponentMask::operator==(
    ComponentMask const &other) const
{
    for (uint64_t i = 0; i < s_block_count; ++i)
    {
        if (m_blocks[i] != other.m_blocks[i])
            return false;
    }
    return true;
}

RECS_FORCEINLINE bool ComponentMask::operator!=(
    ComponentMask const &other) const
{
    for (uint64_t i = 0; i < s_block_count; ++i)
    {
        if (m_blocks[i] != other.m_blocks[i])
            return true;
    }
    return false;
}

RECS_FORCEINLINE std::vector<uint64_t> ComponentMask::typeIds() const
{
    std::vector<uint64_t> ret;
    ret.reserve(s_bit_count);
    for (int64_t block_index = s_block_count - 1; block_index >= 0;
         --block_index)
    {
        block_t const block = m_blocks[block_index];
        for (uint64_t bit_index = 0; bit_index < s_block_bit_count; ++bit_index)
        {

            if ((block & ((block_t)1 << bit_index)) != 0)
                ret.push_back(
                    (s_block_count - block_index - 1) * s_block_bit_count +
                    bit_index);
        }
    }

    return ret;
}

template <typename ComponentT>
    requires ValidComponent<ComponentT>
RECS_FORCEINLINE void ComponentMask::set()
{
    set(TypeId::get<ComponentT>());
}

template <typename ComponentT>
    requires ValidComponent<ComponentT>
RECS_FORCEINLINE void ComponentMask::reset()
{
    reset(TypeId::get<ComponentT>());
}

template <typename ComponentT>
    requires ValidComponent<ComponentT>
RECS_FORCEINLINE bool ComponentMask::test()
{
    return test(TypeId::get<ComponentT>());
}

template <typename ComponentT>
    requires ValidComponent<ComponentT>
RECS_FORCEINLINE uint64_t ComponentMask::count_ones_left_of() const
{
    return count_ones_left_of(TypeId::get<ComponentT>());
}

RECS_FORCEINLINE void ComponentMask::set()
{
    memset(&m_blocks[0], 0xFF, sizeof(block_t) * s_block_count);
}

RECS_FORCEINLINE void ComponentMask::set(uint64_t pos)
{
    assert(pos < s_bit_count);
    uint64_t const block_index = s_block_index(pos);
    assert(block_index < s_block_count);
    block_t const bit_index = s_bit_index(pos);
    block_t &block = m_blocks[block_index];
    block |= (block_t)1 << bit_index;
}

RECS_FORCEINLINE void ComponentMask::reset()
{
    memset(m_blocks, 0x00, sizeof(block_t) * s_block_count);
}

RECS_FORCEINLINE void ComponentMask::reset(uint64_t pos)
{
    assert(pos < s_bit_count);
    uint64_t const block_index = s_block_index(pos);
    assert(block_index < s_block_count);
    block_t const bit_index = s_bit_index(pos);
    block_t &block = m_blocks[block_index];
    block &= ~((block_t)1 << bit_index);
}

RECS_FORCEINLINE bool ComponentMask::test(uint64_t pos) const
{
    assert(pos < s_bit_count);
    uint64_t const block_index = s_block_index(pos);
    assert(block_index < s_block_count);
    block_t const bit_index = s_bit_index(pos);
    block_t const block = m_blocks[block_index];
    bool const ret = (block & ((block_t)1 << bit_index)) != 0;
    return ret;
}

RECS_FORCEINLINE bool ComponentMask::test_all(ComponentMask const &other) const
{
    // TODO:
    // Implement directly? Constructing a new mask seems excessive
    bool const ret = (*this & other) == other;
    return ret;
}

RECS_FORCEINLINE bool ComponentMask::test_any(ComponentMask const &other) const
{
    // TODO:
    // Implement directly? Constructing two new masks seems excessive
    bool const ret = (*this & other) != ComponentMask{};
    return ret;
}

RECS_FORCEINLINE bool ComponentMask::empty() const
{
    uint64_t const count = count_ones();
    return count == 0;
}

RECS_FORCEINLINE uint64_t ComponentMask::count_ones() const
{
    // TODO:
    // Check codegen on clang and msvc, need to hand-vectorize for fast debug?
    uint64_t ret = 0;
    for (uint64_t i = 0; i < s_block_count; ++i)
    {
        block_t const block = m_blocks[i];
        uint64_t const block_clz = std::popcount(block);
        ret += block_clz;
    }
    return ret;
}

[[nodiscard]] RECS_FORCEINLINE uint64_t
ComponentMask::count_ones_left_of(uint64_t pos) const
{
    assert(pos < s_bit_count);
    uint64_t const block_index = s_block_index(pos);
    assert(block_index < s_block_count);
    uint64_t ret = 0;
    for (uint64_t i = s_block_count - 1; i > block_index; --i)
        ret += std::popcount(m_blocks[i]);

    block_t const bit_index = s_bit_index(pos);
    block_t const block = m_blocks[block_index];
    block_t const left_bits_in_block = block & (((uint64_t)1 << bit_index) - 1);
    ret += std::popcount(left_bits_in_block);

    return ret;
}

RECS_FORCEINLINE uint64_t ComponentMask::count_zeros() const
{
    uint64_t const ones = count_ones();
    uint64_t const ret = s_bit_count - ones;
    return ret;
}

RECS_FORCEINLINE uint64_t ComponentMask::count_leading_zeros() const
{
    // TODO:
    // Check codegen on clang and msvc, need to hand-vectorize for fast debug?
    uint64_t ret = 0;
    for (uint64_t i = 0; i < s_block_count; ++i)
    {
        block_t const block = m_blocks[i];
        uint64_t const block_clz = std::countl_zero(block);
        ret += block_clz;
        if (block_clz < s_block_bit_count)
            break;
    }
    return ret;
}

RECS_FORCEINLINE uint64_t ComponentMask::count_leading_ones() const
{
    // TODO:
    // Check codegen on clang and msvc, need to hand-vectorize for fast debug?
    uint64_t ret = 0;
    for (uint64_t i = 0; i < s_block_count; ++i)
    {
        block_t const block = m_blocks[i];
        uint64_t const block_clo = std::countl_one(block);
        ret += block_clo;
        if (block_clo < s_block_bit_count)
            break;
    }
    return ret;
}

RECS_FORCEINLINE uint64_t ComponentMask::count_trailing_zeros() const
{
    // TODO:
    // Check codegen on clang and msvc, need to hand-vectorize for fast debug?
    uint64_t ret = 0;
    for (int64_t i = s_block_count - 1; i >= 0; --i)
    {
        block_t const block = m_blocks[i];
        uint64_t const block_crz = std::countr_zero(block);
        ret += block_crz;
        if (block_crz < s_block_bit_count)
            break;
    }
    return ret;
}

RECS_FORCEINLINE uint64_t ComponentMask::count_trailing_ones() const
{
    // TODO:
    // Check codegen on clang and msvc, need to hand-vectorize for fast debug?
    uint64_t ret = 0;
    for (int64_t i = s_block_count - 1; i >= 0; --i)
    {
        block_t const block = m_blocks[i];
        uint64_t const block_cro = std::countr_one(block);
        ret += block_cro;
        if (block_cro < s_block_bit_count)
            break;
    }
    return ret;
}

RECS_FORCEINLINE uint64_t ComponentMask::hash() const
{
    // Modified PCG-XSL-RR as a stream hash
    // TODO:
    // Is this actually a good hash combine for two 64bit values?
    uint64_t ret = 0;
    for (uint64_t i = 0; i < recs::ComponentMask::s_block_count; ++i)
        ret = std::rotr(ret ^ m_blocks[i], m_blocks[i] >> 58);

    return ret;
}

RECS_FORCEINLINE uint64_t ComponentMask::s_block_index(uint64_t pos)
{
    return s_block_count - (pos / s_block_bit_count) - 1;
}

RECS_FORCEINLINE uint64_t ComponentMask::s_bit_index(uint64_t pos)
{
    return pos % s_block_bit_count;
}

} // namespace recs

template <> struct std::hash<recs::ComponentMask>
{
    std::size_t operator()(recs::ComponentMask const &mask) const noexcept
    {
        return mask.hash();
    }
};
