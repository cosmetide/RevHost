namespace Minecraft.Server.FourKit.Event.Entity;

using FourKitEntity = Minecraft.Server.FourKit.Entity;

/// <summary>
/// Stores data for damage events.
/// </summary>
public class EntityDamageEvent : EntityEvent, Cancellable
{
    /// <summary>
    /// An enum to specify the cause of the damage.
    /// </summary>
    public enum DamageCause
    {
        /// <summary>Damage caused by being in the area when a block explodes.</summary>
        BLOCK_EXPLOSION,
        /// <summary>Damage caused when an entity contacts a block such as a Cactus.</summary>
        CONTACT,
        /// <summary>Custom damage.</summary>
        CUSTOM,
        /// <summary>Damage caused by running out of air while in water.</summary>
        DROWNING,
        /// <summary>Damage caused when an entity attacks another entity.</summary>
        ENTITY_ATTACK,
        /// <summary>Damage caused by being in the area when an entity, such as a Creeper, explodes.</summary>
        ENTITY_EXPLOSION,
        /// <summary>Damage caused when an entity falls a distance greater than 3 blocks.</summary>
        FALL,
        /// <summary>Damage caused by being hit by a falling block which deals damage.</summary>
        FALLING_BLOCK,
        /// <summary>Damage caused by direct exposure to fire.</summary>
        FIRE,
        /// <summary>Damage caused due to burns caused by fire.</summary>
        FIRE_TICK,
        /// <summary>Damage caused by direct exposure to lava.</summary>
        LAVA,
        /// <summary>Damage caused by being struck by lightning.</summary>
        LIGHTNING,
        /// <summary>Damage caused by being hit by a damage potion or spell.</summary>
        MAGIC,
        /// <summary>Damage caused due to a snowman melting.</summary>
        MELTING,
        /// <summary>Damage caused due to an ongoing poison effect.</summary>
        POISON,
        /// <summary>Damage caused when attacked by a projectile.</summary>
        PROJECTILE,
        /// <summary>Damage caused by starving due to having an empty hunger bar.</summary>
        STARVATION,
        /// <summary>Damage caused by being put in a block.</summary>
        SUFFOCATION,
        /// <summary>Damage caused by committing suicide using the command "/kill".</summary>
        SUICIDE,
        /// <summary>Damage caused in retaliation to another attack by the Thorns enchantment.</summary>
        THORNS,
        /// <summary>Damage caused by falling into the void.</summary>
        VOID,
        /// <summary>Damage caused by Wither potion effect.</summary>
        WITHER,
    }

    private readonly DamageCause _cause;
    private double _damage;
    private readonly double _finalDamage;
    private bool _cancel;
    internal EntityDamageEvent(FourKitEntity.Entity damagee, DamageCause cause, double damage)
        : base(damagee)
    {
        _cause = cause;
        _damage = damage;
        _finalDamage = damage;
        _cancel = false;
    }

    /// <summary>
    /// Gets the cause of the damage.
    /// </summary>
    /// <returns>A <see cref="DamageCause"/> value detailing the cause of the damage.</returns>
    public DamageCause getCause() => _cause;

    /// <summary>
    /// Gets the raw amount of damage caused by the event.
    /// </summary>
    /// <returns>The raw amount of damage.</returns>
    public double getDamage() => _damage;

    /// <summary>
    /// Gets the amount of damage caused by the event after all damage
    /// reduction is applied.
    /// </summary>
    /// <returns>The amount of damage after reduction.</returns>
    public double getFinalDamage() => _finalDamage;


    /// <inheritdoc />
    public bool isCancelled() => _cancel;


    /// <inheritdoc />
    public void setCancelled(bool cancel)
    {
        _cancel = cancel;
    }

    /// <summary>
    /// Sets the raw amount of damage caused by the event.
    /// </summary>
    /// <param name="damage">The raw amount of damage.</param>
    public void setDamage(double damage)
    {
        _damage = damage;
    }
}
