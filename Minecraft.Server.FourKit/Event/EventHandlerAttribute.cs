namespace Minecraft.Server.FourKit.Event;

/// <summary>
/// Marks a method inside a <see cref="Listener"/> as an event handler.
/// This class is not named "EventHandler" due to a naming conflict with the existing System.EventHandler
/// </summary>
[AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
public sealed class EventHandlerAttribute : Attribute
{
    /// <summary>
    /// Priority of this handler. Lower values run first.
    /// Default is <see cref="EventPriority.Normal"/>.
    /// </summary>
    public EventPriority Priority { get; set; } = EventPriority.Normal;

    /// <summary>
    /// Whether this handler should be skipped when the event is already
    /// cancelled by a lower-priority handler. Default is <c>false</c>.
    /// </summary>
    public bool IgnoreCancelled { get; set; } = false;
}

/// <summary>
/// Execution priority for event handlers.
/// </summary>
public enum EventPriority
{
    /// <summary>Event call is of very low importance and should be ran first, to allow other plugins to further customise the outcome</summary>
    Lowest = 0,
    /// <summary>Event call is of low importance</summary>
    Low = 1,
    /// <summary>Event call is neither important nor unimportant, and may be ran normally</summary>
    Normal = 2,
    /// <summary>Event call is of high importance</summary>
    High = 3,
    /// <summary>Event call is critical and must have the final say in what happens to the event</summary>
    Highest = 4,
    /// <summary>Event is listened to purely for monitoring the outcome of an event. Should not modify the event.</summary>
    Monitor = 5
}
