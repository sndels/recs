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

using DamagedCharacterEntity = recs::Access::Read<TransformComponent>::Write<
    HealthComponent>::With<CharacterComponent>::As<recs::Entity>;

using DamageSourceQuery = recs::Access::Read<DamageSourceComponent>::Read<
    TransformComponent>::As<recs::Query>;

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

// This just needs to compile
void entity_type_safety(DamagedCharacterEntity dmg)
{
    TransformComponent const &trfn = dmg.getComponent<TransformComponent>();
    (void)trfn;
    HealthComponent const &h = dmg.getComponent<HealthComponent>();
    (void)h;
}

// This just needs to compile
void query_type_safety(DamageSourceQuery dmg)
{
    for (auto &entity : dmg)
    {
        TransformComponent const &trfn =
            entity.getComponent<TransformComponent>();
        (void)trfn;
        DamageSourceComponent const &dmg =
            entity.getComponent<DamageSourceComponent>();
        (void)dmg;
    }
}
