namespace Minecraft.Server.FourKit.Event.Entity;

using FourKitEntity = Minecraft.Server.FourKit.Entity;

/// <summary>
/// Represents an Entity-related event.
/// </summary>
public abstract class EntityEvent : Event
{
    protected FourKitEntity.Entity entity;

    protected EntityEvent(FourKitEntity.Entity what)
    {
        entity = what;
    }

    /// <summary>
    /// Returns the Entity involved in this event.
    /// </summary>
    /// <returns>Entity who is involved in this event.</returns>
    public FourKitEntity.Entity getEntity() => entity;

    /// <summary>
    /// Gets the EntityType of the Entity involved in this event.
    /// </summary>
    /// <returns>EntityType of the Entity involved in this event.</returns>
    public FourKitEntity.EntityType getEntityType() => entity.getType();
}
