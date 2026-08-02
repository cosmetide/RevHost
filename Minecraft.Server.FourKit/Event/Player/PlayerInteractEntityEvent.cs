namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Represents an event that is called when a player right clicks an entity.
/// </summary>
public class PlayerInteractEntityEvent : PlayerEvent, Cancellable
{
    /// <summary>The entity that was right-clicked.</summary>
    protected Entity clickedEntity;

    private bool _cancelled;

    internal PlayerInteractEntityEvent(Player who, Entity clickedEntity)
        : base(who)
    {
        this.clickedEntity = clickedEntity;
    }

    /// <inheritdoc/>
    public bool isCancelled() => _cancelled;

    /// <inheritdoc/>
    public void setCancelled(bool cancel) => _cancelled = cancel;

    /// <summary>
    /// Gets the entity that was right-clicked by the player.
    /// </summary>
    /// <returns>entity right clicked by player</returns>
    public Entity getRightClicked() => clickedEntity;
}
