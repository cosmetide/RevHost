namespace Minecraft.Server.FourKit.Event;

/// <summary>
/// Interface for events that can be cancelled by a plugin.
/// When cancelled, the server will skip the default action.
/// </summary>
public interface Cancellable
{
    /// <summary>Gets whether this event is cancelled.</summary>
    bool isCancelled();

    /// <summary>Sets whether this event is cancelled.</summary>
    void setCancelled(bool cancel);
}
