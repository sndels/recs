#include <catch2/catch_test_macros.hpp>

#include <recs/access.hpp>

namespace
{
struct TransformComponent
{
    float trfn[12]{};
};
struct HealthComponent
{
    float health{0.f};
};
struct CharacterComponent
{
};
struct DamageSourceComponent
{
    float position[3]{};
    float damageOverTime{0.f};
};

using DamagedCharacterAccesses = recs::Access::Read<TransformComponent>::Write<
    HealthComponent>::With<CharacterComponent>;
using DamagedCharacterEntity = DamagedCharacterAccesses::As<recs::Entity>;
using DamagedCharacterQuery = DamagedCharacterAccesses::As<recs::Query>;
using DamagedCharacterQueryIterator =
    DamagedCharacterAccesses::As<recs::QueryIterator>;

using DamageSourceAccesses =
    recs::Access::Read<DamageSourceComponent>::Read<TransformComponent>;
using DamageSourceEntity = DamageSourceAccesses::As<recs::Entity>;
using DamageSourceQuery = DamageSourceAccesses::As<recs::Query>;
using DamageSourceQueryIterator = DamageSourceAccesses::As<recs::QueryIterator>;

} // namespace

// Basic access type tests
static_assert(
    recs::ReadAccessesType<TransformComponent, HealthComponent>::contains<
        TransformComponent>());
static_assert(
    recs::ReadAccessesType<TransformComponent, HealthComponent>::contains<
        HealthComponent>());
static_assert(
    !recs::ReadAccessesType<TransformComponent, HealthComponent>::contains<
        CharacterComponent>());

static_assert(
    recs::WriteAccessesType<TransformComponent, HealthComponent>::contains<
        TransformComponent>());
static_assert(
    recs::WriteAccessesType<TransformComponent, HealthComponent>::contains<
        HealthComponent>());
static_assert(
    !recs::WriteAccessesType<TransformComponent, HealthComponent>::contains<
        CharacterComponent>());

static_assert(
    recs::WithAccessesType<TransformComponent, HealthComponent>::contains<
        TransformComponent>());
static_assert(
    recs::WithAccessesType<TransformComponent, HealthComponent>::contains<
        HealthComponent>());
static_assert(
    !recs::WithAccessesType<TransformComponent, HealthComponent>::contains<
        CharacterComponent>());

TEST_CASE("Entity")
{
    recs::ComponentStorage cs;

    recs::EntityId const e0 = cs.addEntity();
    cs.addComponent(
        e0, TransformComponent{
                .trfn = {1.f, 2.f, 3.f},
            });
    cs.addComponent(
        e0, HealthComponent{
                .health = 99.f,
            });
    cs.addComponent(e0, CharacterComponent{});

    DamagedCharacterEntity dmg{cs, e0};
    TransformComponent const &trfn = dmg.getComponent<TransformComponent>();
    REQUIRE(trfn.trfn[0] == 1.f);
    REQUIRE(trfn.trfn[1] == 2.f);
    REQUIRE(trfn.trfn[2] == 3.f);
    HealthComponent const &h = dmg.getComponent<HealthComponent>();
    REQUIRE(h.health == 99.f);

    {
        recs::ComponentMask referenceMask;
        referenceMask.set(recs::TypeId::get<TransformComponent>());
        referenceMask.set(recs::TypeId::get<HealthComponent>());
        referenceMask.set(recs::TypeId::get<CharacterComponent>());

        recs::ComponentMask const mask = dmg.accessMask();
        REQUIRE(referenceMask == mask);
    }

    {
        recs::ComponentMask referenceMask;
        referenceMask.set(recs::TypeId::get<HealthComponent>());

        recs::ComponentMask const mask = dmg.writeAccessMask();
        REQUIRE(referenceMask == mask);
    }
}

TEST_CASE("Query")
{
    recs::ComponentStorage cs;

    recs::EntityId const e0 = cs.addEntity();
    cs.addComponent(
        e0, TransformComponent{
                .trfn = {1.f, 2.f, 3.f},
            });
    cs.addComponent(
        e0, DamageSourceComponent{
                .damageOverTime = 99.f,
            });

    recs::EntityId const e1 = cs.addEntity();
    cs.addComponent(
        e1, TransformComponent{
                .trfn = {10.f, 20.f, 30.f},
            });
    cs.addComponent(
        e1, DamageSourceComponent{
                .damageOverTime = 9900.f,
            });

    recs::EntityId const e2 = cs.addEntity();
    cs.addComponent(
        e2, TransformComponent{
                .trfn = {100.f, 200.f, 300.f},
            });
    cs.addComponent(
        e2, DamageSourceComponent{
                .damageOverTime = 990000.f,
            });

    DamageSourceQuery q{cs.getEntities(DamageSourceQuery::accessMask())};
    {
        DamageSourceQueryIterator iter = q.begin();

        DamageSourceQueryIterator const end_iter = q.end();
        REQUIRE(iter != end_iter);
        REQUIRE(iter->getComponent<TransformComponent>().trfn[0] == 1.f);
        REQUIRE(iter->getComponent<TransformComponent>().trfn[1] == 2.f);
        REQUIRE(iter->getComponent<TransformComponent>().trfn[2] == 3.f);
        REQUIRE(
            iter->getComponent<DamageSourceComponent>().damageOverTime == 99.f);
        ++iter;
        REQUIRE(iter != end_iter);
        REQUIRE(iter->getComponent<TransformComponent>().trfn[0] == 10.f);
        REQUIRE(iter->getComponent<TransformComponent>().trfn[1] == 20.f);
        REQUIRE(iter->getComponent<TransformComponent>().trfn[2] == 30.f);
        REQUIRE(
            iter->getComponent<DamageSourceComponent>().damageOverTime ==
            9900.f);
        ++iter;
        REQUIRE(iter != end_iter);
        REQUIRE(iter->getComponent<TransformComponent>().trfn[0] == 100.f);
        REQUIRE(iter->getComponent<TransformComponent>().trfn[1] == 200.f);
        REQUIRE(iter->getComponent<TransformComponent>().trfn[2] == 300.f);
        REQUIRE(
            iter->getComponent<DamageSourceComponent>().damageOverTime ==
            990000.f);
        ++iter;
        REQUIRE(iter == end_iter);
    }
    TransformComponent trfn_sum;
    DamageSourceComponent dmg_sum;
    for (DamageSourceEntity entity : q)
    {
        TransformComponent const &trfn =
            entity.getComponent<TransformComponent>();
        trfn_sum.trfn[0] += trfn.trfn[0];
        trfn_sum.trfn[1] += trfn.trfn[1];
        trfn_sum.trfn[2] += trfn.trfn[2];
        DamageSourceComponent const &dmg =
            entity.getComponent<DamageSourceComponent>();
        dmg_sum.damageOverTime += dmg.damageOverTime;
    }
    REQUIRE(trfn_sum.trfn[0] == 111.f);
    REQUIRE(trfn_sum.trfn[1] == 222.f);
    REQUIRE(trfn_sum.trfn[2] == 333.f);
    REQUIRE(dmg_sum.damageOverTime == 999999.f);

    {
        recs::ComponentMask referenceMask;
        referenceMask.set(recs::TypeId::get<TransformComponent>());
        referenceMask.set(recs::TypeId::get<DamageSourceComponent>());
        // TODO: Some 'write' component
        // TODO: Some 'with' component

        recs::ComponentMask const mask = q.accessMask();
        REQUIRE(referenceMask == mask);
    }

    {
        recs::ComponentMask referenceMask;
        // TODO: Some 'write' component

        recs::ComponentMask const mask = q.writeAccessMask();
        REQUIRE(referenceMask == mask);
    }
}
