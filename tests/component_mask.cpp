#include <catch2/catch_test_macros.hpp>

#include "recs/component_mask.hpp"

#include <algorithm>

namespace
{

uint64_t const s_bit_count = recs::ComponentMask::s_bit_count;

}

TEST_CASE("ComponentMask::set/reset")
{

    recs::ComponentMask mask;
    REQUIRE(mask.count_ones() == 0);
    REQUIRE(mask.count_zeros() == s_bit_count);
    REQUIRE(mask.count_leading_zeros() == s_bit_count);
    REQUIRE(mask.count_leading_ones() == 0);
    REQUIRE(mask.count_trailing_zeros() == s_bit_count);
    REQUIRE(mask.count_trailing_ones() == 0);

    for (uint64_t i = 0; i < s_bit_count; ++i)
    {
        mask.reset();
        mask.set(i);
        REQUIRE(mask.test(i));
        REQUIRE(mask.count_ones() == 1);
        REQUIRE(mask.count_zeros() == s_bit_count - 1);
        REQUIRE(mask.count_trailing_zeros() == i);
        REQUIRE(mask.count_trailing_ones() == (i == 0 ? 1 : 0));
        REQUIRE(mask.count_leading_zeros() == s_bit_count - i - 1);
        REQUIRE(mask.count_leading_ones() == (i == (s_bit_count - 1) ? 1 : 0));
        mask.set(i);
        REQUIRE(mask.test(i));
        REQUIRE(mask.count_ones() == 1);
    }

    for (uint64_t i = 0; i < s_bit_count; ++i)
    {
        mask.set();
        mask.reset(i);
        REQUIRE(!mask.test(i));
        REQUIRE(mask.count_ones() == s_bit_count - 1);
        REQUIRE(mask.count_zeros() == 1);
        REQUIRE(mask.count_trailing_zeros() == (i == 0 ? 1 : 0));
        REQUIRE(mask.count_trailing_ones() == i);
        REQUIRE(mask.count_leading_zeros() == (i == (s_bit_count - 1) ? 1 : 0));
        REQUIRE(mask.count_leading_ones() == s_bit_count - i - 1);
        mask.reset(i);
        REQUIRE(!mask.test(i));
        REQUIRE(mask.count_zeros() == 1);
    }

    mask.reset();
    mask.set(700);
    mask.set(501);
    mask.set(500);
    mask.set(499);
    mask.set(456);
    mask.set(311);
    mask.set(154);
    mask.set(4);
    REQUIRE(mask.count_ones_left_of(500) == 5);
}

TEST_CASE("ComponentMask::comparisons")
{
    recs::ComponentMask mask1;
    recs::ComponentMask mask2;
    REQUIRE(mask1 == mask2);
    REQUIRE(mask2 == mask1);
    REQUIRE(!(mask1 != mask2));
    REQUIRE(!(mask2 != mask1));
    mask1.set();
    REQUIRE(mask1 != mask2);
    REQUIRE(mask2 != mask1);
    REQUIRE(!(mask1 == mask2));
    REQUIRE(!(mask2 == mask1));
    mask1.reset();

    for (uint64_t i = 0; i < s_bit_count; i += 2)
    {
        mask1.set(i);
        mask2.set(i);
        REQUIRE(mask1 == mask2);
        REQUIRE(mask2 == mask1);
        REQUIRE(!(mask1 != mask2));
        REQUIRE(!(mask2 != mask1));
    }

    for (uint64_t i = 1; i < s_bit_count; i += 2)
    {
        mask2.set(i);
        REQUIRE(mask1 != mask2);
        REQUIRE(mask2 != mask1);
        REQUIRE(!mask1.test_all(mask2));
        REQUIRE(mask2.test_all(mask1));
        REQUIRE(!(mask1 == mask2));
        REQUIRE(!(mask2 == mask1));
        mask2.reset(i);
        REQUIRE(mask1 == mask2);
        REQUIRE(mask2 == mask1);
        REQUIRE(!(mask1 != mask2));
        REQUIRE(!(mask2 != mask1));
    }
}

TEST_CASE("ComponentMask::and")
{
    recs::ComponentMask mask1;
    recs::ComponentMask mask2;
    recs::ComponentMask mask3;
    mask1.set(100);
    mask2.set(100);
    mask2.set(900);
    mask3.set(800);
    REQUIRE((mask1 & mask2).count_ones() == 1);
    REQUIRE((mask1 & mask2) == mask1);
    REQUIRE((mask3 & mask2).count_ones() == 0);
    REQUIRE((mask2 & mask2).count_ones() == 2);
    REQUIRE((mask2 & mask2) == mask2);
}

TEST_CASE("ComponentMask::set<T>/reset<T>")
{
    recs::ComponentMask mask1;
    recs::ComponentMask mask2;
    mask1.set<int>();
    mask2.set<char>();
    REQUIRE(mask1.count_ones() == 1);
    REQUIRE(mask2.count_ones() == 1);
    REQUIRE(mask1 != mask2);
    mask1.set<char>();
    mask2.set<int>();
    REQUIRE(mask1.count_ones() == 2);
    REQUIRE(mask2.count_ones() == 2);
    REQUIRE(mask1 == mask2);
}

TEST_CASE("ComponentMask::hash")
{
    recs::ComponentMask mask1;
    recs::ComponentMask mask2;
    mask1.set(100);
    mask2.set(800);
    // This doesn't guarantee a quality hash but at least it should work
    REQUIRE(mask1.hash() == mask1.hash());
    REQUIRE(mask2.hash() == mask2.hash());
    REQUIRE(mask1.hash() != mask2.hash());
}
