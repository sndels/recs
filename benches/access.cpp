#include <celero/Celero.h>

#include <recs/access.hpp>
#include <recs/naive_access.hpp>

#include <cstdlib>
#include <vector>

namespace
{

template <class ComponentStorage>
struct HomogeneousEntitiesFixture : public celero::TestFixture
{
    HomogeneousEntitiesFixture() = default;

    virtual void setUp(const celero::TestFixture::ExperimentValue
                           * /*experimentValue*/) override
    {
        for (recs::EntityId const id : ids)
            cs.removeEntity(id);
        ids.clear();

        for (size_t i = 0; i < recs::EntitiesChunk::s_max_entities; ++i)
        {
            recs::EntityId const id = cs.addEntity();
            cs.addComponent(id, rand());
            ids.push_back(id);
        }
    }

    std::vector<recs::EntityId> ids;
    ComponentStorage cs;
};

template <class ComponentStorage>
struct HeterogeneousEntitiesFixture : public celero::TestFixture
{
    HeterogeneousEntitiesFixture() = default;

    virtual void setUp(const celero::TestFixture::ExperimentValue
                           * /*experimentValue*/) override
    {
        for (recs::EntityId const id : ids)
            cs.removeEntity(id);
        ids.clear();

        for (size_t i = 0; i < recs::EntitiesChunk::s_max_entities / 2; ++i)
        {
            recs::EntityId id = cs.addEntity();
            cs.addComponent(id, rand());
            cs.addComponent(id, (uint8_t)0);
            ids.push_back(id);

            id = cs.addEntity();
            cs.addComponent(id, rand());
            cs.addComponent(id, (uint16_t)0);
            ids.push_back(id);
        }
    }

    std::vector<recs::EntityId> ids;
    ComponentStorage cs;
};

template <class ComponentStorage>
struct HomogeneousEntitiesEveryOtherMissingFixture : public celero::TestFixture
{
    HomogeneousEntitiesEveryOtherMissingFixture() = default;

    virtual void setUp(const celero::TestFixture::ExperimentValue
                           * /*experimentValue*/) override
    {
        for (recs::EntityId const id : ids)
            cs.removeEntity(id);
        ids.clear();

        std::vector<recs::EntityId> tmp;
        for (size_t i = 0; i < recs::EntitiesChunk::s_max_entities; ++i)
        {
            recs::EntityId id = cs.addEntity();
            cs.addComponent(id, rand());
            ids.push_back(id);
            id = cs.addEntity();
            cs.addComponent(id, rand());
            tmp.push_back(id);
        }

        for (recs::EntityId id : tmp)
            cs.removeEntity(id);
    }

    std::vector<recs::EntityId> ids;
    ComponentStorage cs;
};

template <class ComponentStorage>
struct HeterogeneousEntitiesEveryOtherMissingFixture
: public celero::TestFixture
{
    HeterogeneousEntitiesEveryOtherMissingFixture() = default;

    virtual void setUp(const celero::TestFixture::ExperimentValue
                           * /*experimentValue*/) override
    {
        for (recs::EntityId const id : ids)
            cs.removeEntity(id);
        ids.clear();

        std::vector<recs::EntityId> tmp;
        for (size_t i = 0; i < recs::EntitiesChunk::s_max_entities / 2; ++i)
        {
            recs::EntityId id = cs.addEntity();
            cs.addComponent(id, rand());
            cs.addComponent(id, (uint8_t)0);
            ids.push_back(id);
            id = cs.addEntity();
            cs.addComponent(id, rand());
            cs.addComponent(id, (uint8_t)0);
            tmp.push_back(id);

            id = cs.addEntity();
            cs.addComponent(id, rand());
            cs.addComponent(id, (uint16_t)0);
            ids.push_back(id);
            id = cs.addEntity();
            cs.addComponent(id, rand());
            cs.addComponent(id, (uint16_t)0);
            tmp.push_back(id);
        }

        for (recs::EntityId id : tmp)
            cs.removeEntity(id);
    }

    std::vector<recs::EntityId> ids;
    ComponentStorage cs;
};

template <class ComponentStorage>
struct HomogeneousEntitiesBadHoleFixture : public celero::TestFixture
{
    HomogeneousEntitiesBadHoleFixture() = default;

    virtual void setUp(const celero::TestFixture::ExperimentValue
                           * /*experimentValue*/) override
    {
        for (recs::EntityId const id : ids)
            cs.removeEntity(id);
        ids.clear();

        std::vector<recs::EntityId> tmp;
        recs::EntityId id = cs.addEntity();
        cs.addComponent(id, rand());
        ids.push_back(id);
        for (size_t i = 0; i < recs::EntitiesChunk::s_max_entities - 2; ++i)
        {
            id = cs.addEntity();
            cs.addComponent(id, rand());
            tmp.push_back(id);
        }
        id = cs.addEntity();
        cs.addComponent(id, rand());
        ids.push_back(id);

        for (recs::EntityId id : tmp)
            cs.removeEntity(id);
    }

    std::vector<recs::EntityId> ids;
    ComponentStorage cs;
};

template <class ComponentStorage>
struct HeterogeneousEntitiesBadHoleFixture : public celero::TestFixture
{
    HeterogeneousEntitiesBadHoleFixture() = default;

    virtual void setUp(const celero::TestFixture::ExperimentValue
                           * /*experimentValue*/) override
    {
        for (recs::EntityId const id : ids)
            cs.removeEntity(id);
        ids.clear();

        std::vector<recs::EntityId> tmp;
        recs::EntityId id = cs.addEntity();
        cs.addComponent(id, rand());
        cs.addComponent(id, (uint8_t)0);
        ids.push_back(id);
        id = cs.addEntity();
        cs.addComponent(id, rand());
        cs.addComponent(id, (uint16_t)0);
        ids.push_back(id);
        for (size_t i = 0; i < (recs::EntitiesChunk::s_max_entities / 2) - 2;
             ++i)
        {
            id = cs.addEntity();
            cs.addComponent(id, rand());
            cs.addComponent(id, (uint8_t)0);
            tmp.push_back(id);
            id = cs.addEntity();
            cs.addComponent(id, rand());
            cs.addComponent(id, (uint16_t)0);
            tmp.push_back(id);
        }
        id = cs.addEntity();
        cs.addComponent(id, rand());
        cs.addComponent(id, (uint8_t)0);
        ids.push_back(id);
        id = cs.addEntity();
        cs.addComponent(id, rand());
        cs.addComponent(id, (uint16_t)0);
        ids.push_back(id);

        for (recs::EntityId id : tmp)
            cs.removeEntity(id);
    }

    std::vector<recs::EntityId> ids;
    ComponentStorage cs;
};

struct IntArrayFixture : public celero::TestFixture
{
    IntArrayFixture() = default;

    virtual void setUp(const celero::TestFixture::ExperimentValue
                           * /*experimentValue*/) override
    {
        data.clear();
        for (size_t i = 0; i < recs::EntitiesChunk::s_max_entities; ++i)
            data.push_back(rand());
    }

    std::vector<int> data;
};

struct TwoIntArrayFixture : public celero::TestFixture
{
    TwoIntArrayFixture() = default;

    virtual void setUp(const celero::TestFixture::ExperimentValue
                           * /*experimentValue*/) override
    {
        data1.clear();
        data2.clear();
        for (size_t i = 0; i < recs::EntitiesChunk::s_max_entities / 2; ++i)
        {
            data1.push_back(rand());
            data2.push_back(rand());
        }
    }

    std::vector<int> data1;
    std::vector<int> data2;
};

using NaiveIntQuery = recs::Access::Read<int>::As<recs::naive::Query>;
using IntQuery = recs::Access::Read<int>::As<recs::Query>;

} // namespace

BASELINE_F(
    queryHomogeneous, Hashmap,
    HomogeneousEntitiesFixture<recs::naive::ComponentStorage>, 30, 100'000)
{
    NaiveIntQuery q{cs.getEntities(NaiveIntQuery::accessMask())};
    int sum = 0;
    for (recs::naive::Entity e : q)
        celero::DoNotOptimizeAway(sum += e.getComponent<int>());
}

BENCHMARK_F(
    queryHomogeneous, Chunks,
    HomogeneousEntitiesFixture<recs::ComponentStorage>, 30, 100'000)
{
    IntQuery q{cs.getEntities(IntQuery::accessMask())};
    int sum = 0;
    for (recs::Entity e : q)
        celero::DoNotOptimizeAway(sum += e.getComponent<int>());
}

BENCHMARK_F(queryHomogeneous, FlatArray, IntArrayFixture, 30, 100'000)
{
    int sum = 0;
    for (int v : data)
        celero::DoNotOptimizeAway(sum += v);
}

BASELINE_F(
    queryHeterogeneous, Hashmap,
    HeterogeneousEntitiesFixture<recs::naive::ComponentStorage>, 30, 100'000)
{
    NaiveIntQuery q{cs.getEntities(NaiveIntQuery::accessMask())};
    int sum = 0;
    for (recs::naive::Entity e : q)
        celero::DoNotOptimizeAway(sum += e.getComponent<int>());
}

BENCHMARK_F(
    queryHeterogeneous, Chunks,
    HeterogeneousEntitiesFixture<recs::ComponentStorage>, 30, 100'000)
{
    IntQuery q{cs.getEntities(IntQuery::accessMask())};
    int sum = 0;
    for (recs::Entity e : q)
        celero::DoNotOptimizeAway(sum += e.getComponent<int>());
}

BENCHMARK_F(queryHeterogeneous, FlatArray, TwoIntArrayFixture, 30, 100'000)
{
    int sum = 0;
    for (int v : data1)
        celero::DoNotOptimizeAway(sum += v);
    for (int v : data2)
        celero::DoNotOptimizeAway(sum += v);
}

BASELINE_F(
    queryHomogeneousEveryOtherMissing, Hashmap,
    HomogeneousEntitiesEveryOtherMissingFixture<recs::naive::ComponentStorage>,
    30, 100'000)
{
    NaiveIntQuery q{cs.getEntities(NaiveIntQuery::accessMask())};
    int sum = 0;
    for (recs::naive::Entity e : q)
        celero::DoNotOptimizeAway(sum += e.getComponent<int>());
}

BENCHMARK_F(
    queryHomogeneousEveryOtherMissing, Chunks,
    HomogeneousEntitiesEveryOtherMissingFixture<recs::ComponentStorage>, 30,
    100'000)
{
    IntQuery q{cs.getEntities(IntQuery::accessMask())};
    int sum = 0;
    for (recs::Entity e : q)
        celero::DoNotOptimizeAway(sum += e.getComponent<int>());
}

BASELINE_F(
    queryHeterogeneousEveryOtherMissing, Hashmap,
    HeterogeneousEntitiesEveryOtherMissingFixture<
        recs::naive::ComponentStorage>,
    30, 100'000)
{
    NaiveIntQuery q{cs.getEntities(NaiveIntQuery::accessMask())};
    int sum = 0;
    for (recs::naive::Entity e : q)
        celero::DoNotOptimizeAway(sum += e.getComponent<int>());
}

BENCHMARK_F(
    queryHeterogeneousEveryOtherMissing, Chunks,
    HeterogeneousEntitiesEveryOtherMissingFixture<recs::ComponentStorage>, 30,
    100'000)
{
    IntQuery q{cs.getEntities(IntQuery::accessMask())};
    int sum = 0;
    for (recs::Entity e : q)
        celero::DoNotOptimizeAway(sum += e.getComponent<int>());
}

BASELINE_F(
    queryHomogeneousBadHole, Hashmap,
    HomogeneousEntitiesBadHoleFixture<recs::naive::ComponentStorage>, 30,
    100'000)
{
    NaiveIntQuery q{cs.getEntities(NaiveIntQuery::accessMask())};
    int sum = 0;
    for (recs::naive::Entity e : q)
        celero::DoNotOptimizeAway(sum += e.getComponent<int>());
}

BENCHMARK_F(
    queryHomogeneousBadHole, Chunks,
    HomogeneousEntitiesBadHoleFixture<recs::ComponentStorage>, 30, 100'000)
{
    IntQuery q{cs.getEntities(IntQuery::accessMask())};
    int sum = 0;
    for (recs::Entity e : q)
        celero::DoNotOptimizeAway(sum += e.getComponent<int>());
}

BASELINE_F(
    queryHeterogeneousBadHole, Hashmap,
    HeterogeneousEntitiesBadHoleFixture<recs::naive::ComponentStorage>, 30,
    100'000)
{
    NaiveIntQuery q{cs.getEntities(NaiveIntQuery::accessMask())};
    int sum = 0;
    for (recs::naive::Entity e : q)
        celero::DoNotOptimizeAway(sum += e.getComponent<int>());
}

BENCHMARK_F(
    queryHeterogeneousBadHole, Chunks,
    HeterogeneousEntitiesBadHoleFixture<recs::ComponentStorage>, 30, 100'000)
{
    IntQuery q{cs.getEntities(IntQuery::accessMask())};
    int sum = 0;
    for (recs::Entity e : q)
        celero::DoNotOptimizeAway(sum += e.getComponent<int>());
}
