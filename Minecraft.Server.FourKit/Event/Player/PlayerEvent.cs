namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Base class for events related to a <see cref="FourKit.Player"/>.
/// </summary>
public abstract class PlayerEvent : Event
{
    private readonly Player _player;

    internal protected PlayerEvent(Player player)
    {
        _player = player;
    }

    /// <summary>Returns the player involved in this event.</summary>
    public Player getPlayer() => _player;
}
