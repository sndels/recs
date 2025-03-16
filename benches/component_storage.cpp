#include <celero/Celero.h>

#include <recs/component_storage.hpp>
#include <recs/naive_component_storage.hpp>

#include <vector>

namespace
{

template <class ComponentStorage>
struct PreallocateSingleEntityFixture : public celero::TestFixture
{
    PreallocateSingleEntityFixture() = default;

    virtual void setUp(const celero::TestFixture::ExperimentValue
                           * /*experimentValue*/) override
    {
        recs::EntityId const id = cs.addEntity();
        cs.addComponent(id, 123u);
        cs.removeEntity(id);
    }

    ComponentStorage cs;
};

template <class ComponentStorage>
struct PreaddSingleEntityFixture : public celero::TestFixture
{
    PreaddSingleEntityFixture() = default;

    virtual void setUp(const celero::TestFixture::ExperimentValue
                           * /*experimentValue*/) override
    {
        stale_id = cs.addEntity();
        cs.removeEntity(stale_id);
        valid_id = cs.addEntity();
    }

    ComponentStorage cs;
    recs::EntityId stale_id;
    recs::EntityId valid_id;
};

template <typename T> struct mat4x4
{
    T m[16];

    mat4x4(T v)
    {
        for (int i = 0; i < 16; ++i)
            m[i] = v;
    }
};

} // namespace

BASELINE_F(
    insertSingleComponent, Hashmap,
    PreallocateSingleEntityFixture<recs::naive::ComponentStorage>, 30, 10'000)
{
    recs::EntityId const id = cs.addEntity();
    cs.addComponent(id, 123u);
    cs.removeEntity(id);
}

BENCHMARK_F(
    insertSingleComponent, Chunk,
    PreallocateSingleEntityFixture<recs::ComponentStorage>, 30, 10'000)
{
    recs::EntityId const id = cs.addEntity();
    cs.addComponent(id, 123u);
    cs.removeEntity(id);
}

BASELINE_F(
    insertLargeSeparate, Hashmap,
    PreallocateSingleEntityFixture<recs::naive::ComponentStorage>, 30, 10'000)
{
    recs::EntityId const id = cs.addEntity();
    cs.addComponent(id, 123u);
    cs.addComponent(id, mat4x4<float>{1.f});
    cs.addComponent(id, mat4x4<double>{1.0});
    cs.addComponent(id, mat4x4<uint64_t>{1});
    cs.removeEntity(id);
}

BENCHMARK_F(
    insertLargeSeparate, Chunk,
    PreallocateSingleEntityFixture<recs::ComponentStorage>, 30, 10'000)
{
    recs::EntityId const id = cs.addEntity();
    cs.addComponent(id, 123u);
    cs.addComponent(id, mat4x4<float>{1.f});
    cs.addComponent(id, mat4x4<double>{1.0});
    cs.addComponent(id, mat4x4<uint64_t>{1});
    cs.removeEntity(id);
}

BASELINE_F(
    isValidValid, Hashmap,
    PreaddSingleEntityFixture<recs::naive::ComponentStorage>, 30, 10'000)
{
    celero::DoNotOptimizeAway(cs.isValid(valid_id));
}

BENCHMARK_F(
    isValidValid, Chunk, PreaddSingleEntityFixture<recs::ComponentStorage>, 30,
    10'000)
{
    celero::DoNotOptimizeAway(cs.isValid(valid_id));
}

BASELINE_F(
    isValidStale, Hashmap,
    PreaddSingleEntityFixture<recs::naive::ComponentStorage>, 30, 10'000)
{
    celero::DoNotOptimizeAway(cs.isValid(stale_id));
}

BENCHMARK_F(
    isValidStale, Chunk, PreaddSingleEntityFixture<recs::ComponentStorage>, 30,
    10'000)
{
    celero::DoNotOptimizeAway(cs.isValid(stale_id));
}
