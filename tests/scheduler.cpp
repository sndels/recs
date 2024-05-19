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

// DAG (Roots at the top)
//       A   B  C
//      / \ /
//     D   E
//      \ / \ 
//       F   G
static bool s_a_ran = false;
void uintASystem(UintEntity) { s_a_ran = true; }

static bool s_b_ran = false;
void uintBSystem(UintEntity) { s_b_ran = true; }

static bool s_c_ran = false;
void uintCSystem(UintEntity) { s_c_ran = true; }

static bool s_d_ran = false;
static bool s_d_ran_after_a = false;
void uintDSystem(UintEntity)
{
    s_d_ran = true;
    s_d_ran_after_a = s_a_ran;
}

static bool s_e_ran = false;
static bool s_e_ran_after_a_and_b = false;
void uintESystem(UintEntity)
{
    s_e_ran = true;
    s_e_ran_after_a_and_b = s_a_ran && s_b_ran;
}

static bool s_f_ran = false;
static bool s_f_ran_after_d_and_e = false;
void uintFSystem(UintEntity)
{
    s_f_ran = true;
    s_f_ran_after_d_and_e = s_d_ran && s_e_ran;
}

static bool s_g_ran = false;
static bool s_g_ran_after_e = false;
void uintGSystem(UintEntity)
{
    s_g_ran = true;
    s_g_ran_after_e = s_e_ran;
}

struct Dag
{
    recs::SystemRef a;
    recs::SystemRef b;
    recs::SystemRef c;
    recs::SystemRef d;
    recs::SystemRef e;
    recs::SystemRef f;
    recs::SystemRef g;
};

void setUpGraph(Dag const &dag)
{
    dag.d.executeAfter(dag.a);
    dag.e.executeAfter(dag.a).executeAfter(dag.b);
    dag.f.executeAfter(dag.d).executeAfter(dag.e);
    dag.g.executeAfter(dag.e);
}

} // namespace

TEST_CASE("Scheduler basic")
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

    recs::Schedule const schedule = scheduler.buildSchedule();

    s_int_sum = 0;
    s_uint_sum = 0;
    s_combined_sum = 0;
    s_uint_diff = 0;
    schedule.execute(storage);
    REQUIRE(s_int_sum == ref_int_sum);
    REQUIRE(s_uint_sum == ref_uint_sum);
    REQUIRE(s_combined_sum == ref_combined_sum);
    REQUIRE(s_uint_diff == ref_uint_diff);
}

TEST_CASE("Scheduler dependencies")
{
    recs::Scheduler scheduler;
    recs::ComponentStorage storage;

    recs::EntityId const e = storage.addEntity();
    storage.addComponent(e, 0u);

    SECTION("Ordered push")
    {
        Dag dag;
        dag.a = scheduler.registerSystem(uintASystem);
        dag.b = scheduler.registerSystem(uintBSystem);
        dag.c = scheduler.registerSystem(uintCSystem);
        dag.d = scheduler.registerSystem(uintDSystem);
        dag.e = scheduler.registerSystem(uintESystem);
        dag.f = scheduler.registerSystem(uintFSystem);
        dag.g = scheduler.registerSystem(uintGSystem);

        setUpGraph(dag);

        recs::Schedule const schedule = scheduler.buildSchedule();
        s_a_ran = false;
        s_b_ran = false;
        s_c_ran = false;
        s_d_ran = false;
        s_e_ran = false;
        s_f_ran = false;
        s_g_ran = false;
        s_d_ran_after_a = false;
        s_e_ran_after_a_and_b = false;
        s_f_ran_after_d_and_e = false;
        s_g_ran_after_e = false;
        schedule.execute(storage);
        REQUIRE(s_a_ran);
        REQUIRE(s_b_ran);
        REQUIRE(s_c_ran);
        REQUIRE(s_d_ran);
        REQUIRE(s_e_ran);
        REQUIRE(s_f_ran);
        REQUIRE(s_g_ran);
        REQUIRE(s_d_ran_after_a);
        REQUIRE(s_e_ran_after_a_and_b);
        REQUIRE(s_f_ran_after_d_and_e);
        REQUIRE(s_g_ran_after_e);
    }

    SECTION("Reverse push")
    {
        Dag dag;
        dag.g = scheduler.registerSystem(uintGSystem);
        dag.f = scheduler.registerSystem(uintFSystem);
        dag.e = scheduler.registerSystem(uintESystem);
        dag.d = scheduler.registerSystem(uintDSystem);
        dag.c = scheduler.registerSystem(uintCSystem);
        dag.b = scheduler.registerSystem(uintBSystem);
        dag.a = scheduler.registerSystem(uintASystem);

        setUpGraph(dag);

        recs::Schedule const schedule = scheduler.buildSchedule();
        s_a_ran = false;
        s_b_ran = false;
        s_c_ran = false;
        s_d_ran = false;
        s_e_ran = false;
        s_f_ran = false;
        s_g_ran = false;
        s_d_ran_after_a = false;
        s_e_ran_after_a_and_b = false;
        s_f_ran_after_d_and_e = false;
        s_g_ran_after_e = false;
        schedule.execute(storage);
        REQUIRE(s_a_ran);
        REQUIRE(s_b_ran);
        REQUIRE(s_c_ran);
        REQUIRE(s_d_ran);
        REQUIRE(s_e_ran);
        REQUIRE(s_f_ran);
        REQUIRE(s_g_ran);
        REQUIRE(s_d_ran_after_a);
        REQUIRE(s_e_ran_after_a_and_b);
        REQUIRE(s_f_ran_after_d_and_e);
        REQUIRE(s_g_ran_after_e);
    }

    SECTION("Scramble")
    {
        Dag dag;
        dag.f = scheduler.registerSystem(uintFSystem);
        dag.g = scheduler.registerSystem(uintGSystem);
        dag.c = scheduler.registerSystem(uintCSystem);
        dag.e = scheduler.registerSystem(uintESystem);
        dag.a = scheduler.registerSystem(uintASystem);
        dag.b = scheduler.registerSystem(uintBSystem);
        dag.d = scheduler.registerSystem(uintDSystem);

        setUpGraph(dag);

        recs::Schedule const schedule = scheduler.buildSchedule();
        s_a_ran = false;
        s_b_ran = false;
        s_c_ran = false;
        s_d_ran = false;
        s_e_ran = false;
        s_f_ran = false;
        s_g_ran = false;
        s_d_ran_after_a = false;
        s_e_ran_after_a_and_b = false;
        s_f_ran_after_d_and_e = false;
        s_g_ran_after_e = false;
        schedule.execute(storage);
        REQUIRE(s_a_ran);
        REQUIRE(s_b_ran);
        REQUIRE(s_c_ran);
        REQUIRE(s_d_ran);
        REQUIRE(s_e_ran);
        REQUIRE(s_f_ran);
        REQUIRE(s_g_ran);
        REQUIRE(s_d_ran_after_a);
        REQUIRE(s_e_ran_after_a_and_b);
        REQUIRE(s_f_ran_after_d_and_e);
        REQUIRE(s_g_ran_after_e);
    }
}
