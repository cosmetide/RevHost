namespace Minecraft.Server.FourKit.Event.Entity;

using FourKitEntity = Minecraft.Server.FourKit.Entity;

/// <summary>
/// Called when an entity is damaged by an entity.
/// </summary>
public class EntityDamageByEntityEvent : EntityDamageEvent
{
    private readonly FourKitEntity.Entity _damager;
    internal EntityDamageByEntityEvent(FourKitEntity.Entity damager, FourKitEntity.Entity damagee, EntityDamageEvent.DamageCause cause, double damage)
        : base(damagee, cause, damage)
    {
        _damager = damager;
    }

    /// <summary>
    /// Returns the entity that damaged the defender.
    /// </summary>
    /// <returns>The Entity that damaged the defender.</returns>
    public FourKitEntity.Entity getDamager() => _damager;
}
