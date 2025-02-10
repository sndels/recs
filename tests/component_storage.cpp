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
    REQUIRE(ecs.hasComponent<DataF>(e1));
    ecs.addComponent(e0, DataF{4.f});
    REQUIRE(ecs.getComponent<DataF>(e0).f == 4.f);

    ecs.removeEntity(e0);
    e0 = ecs.addEntity();
    REQUIRE(!ecs.hasComponent<DataF>(e0));
    REQUIRE(!ecs.hasComponent<DataI>(e0));
}

// Thanks ChatGPT, first try got almost what's here now
TEST_CASE("getEntities")
{
    recs::ComponentStorage cs;

    SECTION("Add entities and components, retrieve with mask")
    {
        recs::EntityId const e0 = cs.addEntity();
        recs::EntityId const e1 = cs.addEntity();

        // float and uint32_t component
        cs.addComponent(e0, 1.f);
        cs.addComponent(e0, 0u);
        // float component only
        cs.addComponent(e1, 2.f);

        recs::ComponentMask mask;
        mask.set(recs::TypeId::get<float>());
        mask.set(recs::TypeId::get<uint32_t>());

        recs::ComponentStorage::Range const ents = cs.getEntities(mask);

        REQUIRE(ents.size() == 1);
        REQUIRE(ents.getId(0) == e0);
    }

    SECTION("No entities match the mask")
    {
        recs::EntityId const e0 = cs.addEntity();
        recs::EntityId const e1 = cs.addEntity();

        cs.addComponent(e0, 1.f); // float component
        cs.addComponent(e1, 0u);  // uint32_t component

        recs::ComponentMask mask;
        mask.set(recs::TypeId::get<float>());
        mask.set(recs::TypeId::get<uint32_t>());

        recs::ComponentStorage::Range const ents = cs.getEntities(mask);

        REQUIRE(ents.empty());
    }

    SECTION("Retrieve entities with a single component type")
    {
        recs::EntityId const e0 = cs.addEntity();
        recs::EntityId const e1 = cs.addEntity();

        cs.addComponent(e0, 1.f); // float component
        cs.addComponent(e1, 2.f); // float component

        recs::ComponentMask mask;
        mask.set(recs::TypeId::get<float>());

        recs::ComponentStorage::Range const ents = cs.getEntities(mask);

        REQUIRE(ents.size() == 2);
        REQUIRE((ents.getId(0) == e0 || ents.getId(0) == e1));
        REQUIRE((ents.getId(1) == e0 || ents.getId(1) == e1));
    }

    SECTION("Retrieve entities with no components")
    {
        recs::EntityId const e0 = cs.addEntity();
        recs::EntityId const e1 = cs.addEntity();

        cs.addComponent(e0, 1.f); // a component

        recs::ComponentMask mask;

        recs::ComponentStorage::Range const ents = cs.getEntities(mask);

        REQUIRE(ents.size() == 2);
        REQUIRE((ents.getId(0) == e0 || ents.getId(0) == e1));
        REQUIRE((ents.getId(1) == e0 || ents.getId(1) == e1));
    }

    SECTION("Adding and retrieving multiple types of components")
    {
        recs::EntityId const e0 = cs.addEntity();
        recs::EntityId const e1 = cs.addEntity();
        recs::EntityId const e2 = cs.addEntity();

        // float and uint32_t component
        cs.addComponent(e0, 1.f);
        cs.addComponent(e0, 0u);
        // uint32_t component only
        cs.addComponent(e1, 1u);
        // float component only
        cs.addComponent(e2, 3.f);

        recs::ComponentMask mask1;
        mask1.set(recs::TypeId::get<float>());
        mask1.set(recs::TypeId::get<uint32_t>());

        recs::ComponentStorage::Range const ents1 = cs.getEntities(mask1);

        REQUIRE(ents1.size() == 1);
        REQUIRE(ents1.getId(0) == e0);
        REQUIRE(ents1.getComponent<float>(0) == 1.f);
        REQUIRE(ents1.getComponent<uint32_t>(0) == 0u);

        recs::ComponentMask mask2;
        mask2.set(recs::TypeId::get<uint32_t>());

        recs::ComponentStorage::Range const ents2 = cs.getEntities(mask2);

        REQUIRE(ents2.size() == 2);
        REQUIRE((ents2.getId(0) == e0 || ents2.getId(0) == e1));
        REQUIRE((ents2.getId(1) == e0 || ents2.getId(1) == e1));

        recs::ComponentMask mask3;
        mask3.set(recs::TypeId::get<float>());

        recs::ComponentStorage::Range const ents3 = cs.getEntities(mask3);

        REQUIRE(ents3.size() == 2);
        REQUIRE((ents3.getId(0) == e0 || ents3.getId(0) == e2));
        REQUIRE((ents3.getId(1) == e0 || ents3.getId(1) == e2));
    }

    SECTION("Add, remove entities and retrieve with mask")
    {
        recs::EntityId const e0 = cs.addEntity();
        recs::EntityId const e1 = cs.addEntity();

        // float and uint32_t component
        cs.addComponent(e0, 1.f);
        cs.addComponent(e0, 0u);
        // float component only
        cs.addComponent(e1, 2.f);

        // Remove entity e0
        cs.removeEntity(e0);

        recs::ComponentMask mask;
        mask.set(recs::TypeId::get<float>());
        mask.set(recs::TypeId::get<uint32_t>());

        {
            recs::ComponentStorage::Range const ents = cs.getEntities(mask);

            // Entity e0 should not be in the list since it has been removed
            REQUIRE(ents.empty());
        }

        // Add a new entity, presumably reusing the memory for e0
        recs::EntityId const e2 = cs.addEntity();
        (void)e2;

        {
            recs::ComponentStorage::Range const ents = cs.getEntities(mask);

            // The new entity should not be in the list
            REQUIRE(ents.empty());
        }
    }
}
