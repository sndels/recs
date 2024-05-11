# recs

Remedy had this interesting looking API in their GDC 2024 talk "ECS in Practice: The Case Board of 'Alan Wake 2'" and it wasn't obvious to me how it would be implemented in under the hood. Let's try to find out.

There were typed queries with access types for system interfaces:

```
using DamagedCharacterEntity = ecs::Access
    ::Read< TransformComponent >
    ::Write< HealthComponent >
    ::With< CharacterComponent >
    ::As< ecs::Entity >;

using DamageSourceQuery = ecs::Access
    ::Read< DamageSourceComponent >
    ::Read< TransformComponent >
    ::As< ecs::Query >;

void dealDamageOverTimeSystem( DamagedCharacterEntity entity, DamageSourceQuery query, const env::FixedTime& fixedTime )
{
    const auto& characterTransform = entity.getComponent< TransformComponent >();
    auto& characterHealth = entity.getComponent< HealthComponent >();

    for( auto damageSourceEntity : query )
    {
        auto[ damageSource, damageTransform ] = damageSourceEntity;
        if( transform::distance( characterTransform, damageTransform ) < damageSource.distance )
        {
            characterHealth.m_healthValue -= damageSource.m_damageOverTimeValue * fixedTime.m_deltaTime;
        }
    }
}

```

Also a command buffer for delayed operations like adding components:

```
void animateWorldTransform( TransformEntity entity, m::Transform targetWorldTransform, SpringParameters springParameters, ecs::CommandBuffer ecb )
{
    auto& transformComponent = entity.getComponent< component::WorldTransform >();
    if( m::Transform::areNotEqual( transformComponent.value, targetWorldTransform ) )
    {
        // After adding SpringMotion component an entity will be processed by a motion system
        ecb.addComponent( entity, component::SpringMotion{ targetWorldTransform, springParameters } );
    }
}
```

The queries were registered to a job system that overlapped things it could based on rw access and explicit ordering:

```
void FixedUpdateSystemA( Entity entity, QueryA query, const env::FixedUpdate& fixedUpdate ) { /* ... */ }
void FixedUpdateSystemB( Entity entity, QueryB query, const env::FixedUpdate& fixedUpdate ) { /* ... */ }

void registerModule( ECSBuilder& builder )
{
    auto variableUpdate = builder.gameloop()[ sg_svVariableUpdateSystemGroupName ];
    auto fixedUpdate = builder.gameloop()[ sg_svFixedUpdateSystemGroupName ];

    auto variableSystem = varibleUpdate.registerSystem< VariableUpdateSystem >();

    auto systemA = fixedUpdate.registerSystem< FixedUpdateSystemA >();

    auto systemB = fixedUpdate.registerSystem< FixedUpdateSystemB >()
        .order()
        .executeAfter( systemA );
}
```

Note the different update steps that have their own environment structs.
