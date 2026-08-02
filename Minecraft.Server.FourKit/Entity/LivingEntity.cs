namespace Minecraft.Server.FourKit.Entity;

/// <summary>
/// Represents a living entity in the world that has health and can take damage.
/// </summary>
public class LivingEntity : Damageable
{
    private double _eyeHeight = 1.62;

    internal LivingEntity() { }

    internal LivingEntity(int entityId, EntityType entityType, int dimId, double x, double y, double z,
        float health = 20f, float maxHealth = 20f)
    {
        SetEntityIdInternal(entityId);
        SetEntityTypeInternal(entityType);
        SetDimensionInternal(dimId);
        SetLocation(new Location(FourKit.getWorld(dimId), x, y, z));
        if (maxHealth > 0)
            SetMaxHealthInternal(maxHealth);
        SetHealthInternal(health);
    }

    /// <summary>
    /// Gets the height of the living entity's eyes above its <see cref="Location"/>.
    /// </summary>
    /// <returns>The eye height.</returns>
    public double getEyeHeight() => _eyeHeight;

    /// <summary>
    /// Gets the height of the living entity's eyes above its <see cref="Location"/>.
    /// </summary>
    /// <param name="ignoreSneaking">If <c>true</c>, returns the standing eye height regardless of sneak state.</param>
    /// <returns>The eye height.</returns>
    public double getEyeHeight(bool ignoreSneaking)
    {
        if (ignoreSneaking)
            return _eyeHeight;

        // When sneaking the eye height is slightly lower
        return _eyeHeight - 0.08;
    }

    // --- Internal setter used by the bridge ---

    /// <summary>
    /// Updates the eye height. Called internally by the bridge.
    /// </summary>
    /// <param name="eyeHeight">The new eye height.</param>
    internal void SetEyeHeightInternal(double eyeHeight) => _eyeHeight = eyeHeight;
}
