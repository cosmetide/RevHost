namespace Minecraft.Server.FourKit.Event.Inventory;

/// <summary>
/// What the client did to trigger this action (not the result).
/// </summary>
public enum ClickType
{
    /// <summary>The left (or primary) mouse button.</summary>
    LEFT,
    /// <summary>Holding shift while pressing the left mouse button.</summary>
    SHIFT_LEFT,
    /// <summary>The right mouse button.</summary>
    RIGHT,
    /// <summary>Holding shift while pressing the right mouse button.</summary>
    SHIFT_RIGHT,
    /// <summary>Clicking the left mouse button on the grey area around the inventory.</summary>
    WINDOW_BORDER_LEFT,
    /// <summary>Clicking the right mouse button on the grey area around the inventory.</summary>
    WINDOW_BORDER_RIGHT,
    /// <summary>The middle mouse button, or a "scrollwheel click".</summary>
    MIDDLE,
    /// <summary>One of the number keys 1-9, correspond to slots on the hotbar.</summary>
    NUMBER_KEY,
    /// <summary>Pressing the left mouse button twice in quick succession.</summary>
    DOUBLE_CLICK,
    /// <summary>The "Drop" key (defaults to Q).</summary>
    DROP,
    /// <summary>Holding Ctrl while pressing the "Drop" key (defaults to Q).</summary>
    CONTROL_DROP,
    /// <summary>Any action done with the Creative inventory open.</summary>
    CREATIVE,
    /// <summary>A type of inventory manipulation not yet recognized by Bukkit.</summary>
    UNKNOWN,
}

/// <summary>
/// Extension methods for <see cref="ClickType"/>.
/// </summary>
public static class ClickTypeExtensions
{
    /// <summary>
    /// Gets whether this ClickType represents the pressing of a key on a keyboard.
    /// </summary>
    /// <param name="click">The click type.</param>
    /// <returns>true if this ClickType represents the pressing of a key.</returns>
    public static bool isKeyboardClick(this ClickType click)
    {
        return click == ClickType.NUMBER_KEY || click == ClickType.DROP || click == ClickType.CONTROL_DROP;
    }

    /// <summary>
    /// Gets whether this ClickType represents an action that can only be performed
    /// by a Player in creative mode.
    /// </summary>
    /// <param name="click">The click type.</param>
    /// <returns>true if this action requires Creative mode.</returns>
    public static bool isCreativeAction(this ClickType click)
    {
        return click == ClickType.CREATIVE || click == ClickType.MIDDLE;
    }

    /// <summary>
    /// Gets whether this ClickType represents a right click.
    /// </summary>
    /// <param name="click">The click type.</param>
    /// <returns>true if this ClickType represents a right click.</returns>
    public static bool isRightClick(this ClickType click)
    {
        return click == ClickType.RIGHT || click == ClickType.SHIFT_RIGHT;
    }

    /// <summary>
    /// Gets whether this ClickType represents a left click.
    /// </summary>
    /// <param name="click">The click type.</param>
    /// <returns>true if this ClickType represents a left click.</returns>
    public static bool isLeftClick(this ClickType click)
    {
        return click == ClickType.LEFT || click == ClickType.SHIFT_LEFT
            || click == ClickType.DOUBLE_CLICK || click == ClickType.CREATIVE;
    }

    /// <summary>
    /// Gets whether this ClickType indicates that the shift key was pressed
    /// down when the click was made.
    /// </summary>
    /// <param name="click">The click type.</param>
    /// <returns>true if the action uses Shift.</returns>
    public static bool isShiftClick(this ClickType click)
    {
        return click == ClickType.SHIFT_LEFT || click == ClickType.SHIFT_RIGHT;
    }
}
