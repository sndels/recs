#include <catch2/catch_test_macros.hpp>

#include "recs/component_storage.hpp"

namespace
{

struct DataF
{
    float f{0.f};
};

struct DataI
{
    int i{0};
};

struct alignas(32) Aligned
{
    float x;
};

} // namespace

TEST_CASE("ComponentStorage")
{
    recs::ComponentStorage ecs;

    recs::EntityId e0 = ecs.addEntity();
    REQUIRE(ecs.isValid(e0));
    recs::EntityId const e1 = ecs.addEntity();
    REQUIRE(ecs.isValid(e1));
    ecs.removeEntity(e0);
    REQUIRE(!ecs.isValid(e0));
    REQUIRE(ecs.isValid(e1));
    e0 = ecs.addEntity();
    REQUIRE(ecs.isValid(e0));

    // Storage alignment
    ecs.addComponent(e0, (uint16_t)1);
    REQUIRE(
        ((uintptr_t)&ecs.getComponent<uint16_t>(e0)) % alignof(uint16_t) == 0);
    ecs.addComponent(e0, Aligned{123.f});
    REQUIRE(
        ((uintptr_t)&ecs.getComponent<uint16_t>(e0)) % alignof(uint16_t) == 0);
    REQUIRE(
        ((uintptr_t)&ecs.getComponent<Aligned>(e0)) % alignof(Aligned) == 0);
    ecs.addComponent(e1, (uint16_t)2);
    ecs.addComponent(e1, Aligned{456.f});
    REQUIRE(ecs.getComponent<uint16_t>(e0) == 1);
    REQUIRE(ecs.getComponent<uint16_t>(e1) == 2);
    REQUIRE(ecs.getComponent<Aligned>(e0).x == 123.f);
    REQUIRE(ecs.getComponent<Aligned>(e1).x == 456.f);
    ecs.removeComponent<uint16_t>(e0);
    ecs.removeComponent<Aligned>(e0);

    ecs.addComponent(e0, DataF{1.f});
    REQUIRE(ecs.hasComponent<DataF>(e0));
    // Should be able to check for a type that no entity has ever had
    REQUIRE(!ecs.hasComponent<int>(e0));
    ecs.addComponent(e1, DataF{2.f});
    REQUIRE(ecs.hasComponent<DataF>(e1));
    ecs.addComponent(e0, DataI{3});
    REQUIRE(ecs.hasComponent<DataI>(e0));
    REQUIRE(ecs.hasComponents<DataI, DataF>(e0));
    REQUIRE(ecs.hasComponents<DataF, DataI>(e0));
    REQUIRE(!ecs.hasComponent<DataI>(e1));
    REQUIRE(!ecs.hasComponents<DataI, DataF>(e1));
    REQUIRE(!ecs.hasComponents<DataF, DataI>(e1));

    REQUIRE(ecs.getComponent<DataF>(e0).f == 1.f);
    REQUIRE(ecs.getComponent<DataF>(e1).f == 2.f);
    REQUIRE(ecs.getComponent<DataI>(e0).i == 3);
    ecs.removeComponent<DataF>(e0);
    REQUIRE(!ecs.hasComponent<DataF>(e0));
    REQUIRE(ecs.hasComponent<DataI>(e0));
    REQUIRE(ecs.getComponent<DataI>(e0).i == 3);
    REQUIRE(ecs.hasComponent<DataF>(e1));
    ecs.addComponent(e0, DataF{4.f});
    REQUIRE(ecs.getComponent<DataF>(e0).f == 4.f);

    ecs.removeEntity(e0);
    e0 = ecs.addEntity();
    REQUIRE(!ecs.hasComponent<DataF>(e0));
    REQUIRE(!ecs.hasComponent<DataI>(e0));
}
