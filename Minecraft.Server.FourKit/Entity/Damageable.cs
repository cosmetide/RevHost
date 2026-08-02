namespace Minecraft.Server.FourKit.Entity;

/// <summary>
/// Represents an <see cref="Entity"/> that can take damage and has health.
/// </summary>
public class Damageable : Entity
{
    private double _health = 20.0;
    private double _maxHealth = 20.0;
    private readonly double _originalMaxHealth = 20.0;

    /// <summary>
    /// Deals the given amount of damage to this entity.
    /// This calls into the native server to apply real damage.
    /// </summary>
    /// <param name="amount">Amount of damage to deal.</param>
    public void damage(double amount)
    {
        NativeBridge.DamagePlayer?.Invoke(getEntityId(), (float)amount);
    }

    /// <summary>
    /// Gets the entity's health from 0 to <see cref="getMaxHealth"/>, where 0 is dead.
    /// </summary>
    /// <returns>The current health.</returns>
    public double getHealth() => _health;

    /// <summary>
    /// Gets the maximum health this entity has.
    /// </summary>
    /// <returns>The maximum health.</returns>
    public double getMaxHealth() => _maxHealth;

    /// <summary>
    /// Resets the max health to the original amount.
    /// </summary>
    public void resetMaxHealth()
    {
        _maxHealth = _originalMaxHealth;
        if (_health > _maxHealth)
            _health = _maxHealth;
    }

    /// <summary>
    /// Sets the entity's health from 0 to <see cref="getMaxHealth"/>, where 0 is dead.
    /// This calls into the native server to apply the health change.
    /// </summary>
    /// <param name="health">New health value.</param>
    public void setHealth(double health)
    {
        NativeBridge.SetPlayerHealth?.Invoke(getEntityId(), (float)Math.Clamp(health, 0.0, _maxHealth));
    }

    /// <summary>
    /// Sets the maximum health this entity can have.
    /// If the entity's current health exceeds the new maximum, it is clamped.
    /// </summary>
    /// <param name="health">New maximum health value.</param>
    public void setMaxHealth(double health)
    {
        _maxHealth = health;
        if (_health > _maxHealth)
            _health = _maxHealth;
    }

    // --- Internal setter used by the bridge ---

    /// <summary>
    /// Updates health directly. Called internally by the bridge.
    /// </summary>
    /// <param name="health">The new health value.</param>
    internal void SetHealthInternal(double health) => _health = health;

    /// <summary>
    /// Updates max health directly. Called internally by the bridge.
    /// </summary>
    /// <param name="maxHealth">The new max health value.</param>
    internal void SetMaxHealthInternal(double maxHealth) => _maxHealth = maxHealth;
}
