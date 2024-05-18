#include <catch2/catch_test_macros.hpp>

#include "recs/access.hpp"
#include "recs/scheduler.hpp"
#include <cstdlib>

namespace
{

using IntEntity = recs::Access::Read<int32_t>::As<recs::Entity>;
using UintEntity = recs::Access::Read<uint32_t>::As<recs::Entity>;
using UintQuery = recs::Access::Read<uint32_t>::As<recs::Query>;
using CombinedEntity =
    recs::Access::Read<int32_t>::Read<uint32_t>::As<recs::Entity>;

static int32_t s_int_sum = 0;
void intSumSystem(IntEntity e) { s_int_sum += e.getComponent<int32_t>(); }

static uint32_t s_uint_sum = 0;
void uintSumSystem(UintEntity e) { s_uint_sum += e.getComponent<uint32_t>(); }

static uint32_t s_combined_sum = 0;
void combinedSumSystem(CombinedEntity e)
{
    int32_t const i32 = e.getComponent<int32_t>();
    uint32_t const u32 = e.getComponent<uint32_t>();
    s_combined_sum += (uint32_t)i32 + u32;
}

static int32_t s_uint_diff = 0;
void uintDiffSystem(IntEntity e, UintQuery const &q)
{
    int32_t value = e.getComponent<int32_t>();
    for (UintEntity const &qe : q)
        s_uint_diff += value - qe.getComponent<uint32_t>();
}

} // namespace

TEST_CASE("Scheduler")
{
    recs::Scheduler scheduler;
    recs::ComponentStorage storage;

    int32_t ref_int_sum = 0;
    uint32_t ref_uint_sum = 0;
    uint32_t ref_combined_sum = 0;
    std::vector<int32_t> ints;
    std::vector<uint32_t> uints;
    for (int32_t i = 0; i < 1000; ++i)
    {
        recs::EntityId const e = storage.addEntity();

        // TODO: Use catch2 random source?
        float const rng = std::rand() / (float)RAND_MAX;
        if (rng < .3)
        {
            storage.addComponent(e, i);
            ints.push_back(i);
            ref_int_sum += i;
        }
        else if (rng < .6)
        {
            storage.addComponent(e, static_cast<uint32_t>(i));
            uints.push_back(i);
            ref_uint_sum += i;
        }
        else
        {
            storage.addComponent(e, i);
            storage.addComponent(e, static_cast<uint32_t>(i));
            ints.push_back(i);
            uints.push_back(i);
            ref_int_sum += i;
            ref_uint_sum += i;
            ref_combined_sum += i + i;
        }
    }

    int32_t ref_uint_diff = 0;
    for (int32_t i : ints)
    {
        for (uint32_t u : uints)
            ref_uint_diff += i - u;
    }

    scheduler.registerSystem(intSumSystem);
    scheduler.registerSystem(uintSumSystem);
    scheduler.registerSystem(combinedSumSystem);
    scheduler.registerSystem(uintDiffSystem);

    s_int_sum = 0;
    s_uint_sum = 0;
    s_combined_sum = 0;
    s_uint_diff = 0;
    scheduler.executeSystems(storage);
    REQUIRE(s_int_sum == ref_int_sum);
    REQUIRE(s_uint_sum == ref_uint_sum);
    REQUIRE(s_combined_sum == ref_combined_sum);
    REQUIRE(s_uint_diff == ref_uint_diff);
}
