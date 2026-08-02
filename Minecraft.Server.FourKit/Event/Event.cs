namespace Minecraft.Server.FourKit.Event;

/// <summary>
/// Base class for all events dispatched by the server.
/// </summary>
public abstract class Event
{
    /// <summary>Gets the name of this event (defaults to the class name).</summary>
    public virtual string getEventName() => GetType().Name;
}
