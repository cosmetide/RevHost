namespace Minecraft.Server.FourKit.Event.Entity;

using FourKitEntity = Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Thrown whenever a Player dies.
/// </summary>
public class PlayerDeathEvent : EntityDeathEvent
{
    private string _deathMessage;
    private int _newExp;
    private int _newLevel;
    private bool _keepLevel;
    private bool _keepInventory;
    internal PlayerDeathEvent(FourKitEntity.Player player, List<ItemStack> drops, int droppedExp, string deathMessage)
        : this(player, drops, droppedExp, 0, deathMessage)
    {
    }

    /// <summary>
    /// Creates a new <see cref="PlayerDeathEvent"/>.
    /// </summary>
    /// <param name="player">The Player who died.</param>
    /// <param name="drops">The items to drop when the player dies.</param>
    /// <param name="droppedExp">The amount of experience to drop.</param>
    /// <param name="newExp">The new EXP the Player should have at respawn.</param>
    /// <param name="deathMessage">The death message to display.</param>
    public PlayerDeathEvent(FourKitEntity.Player player, List<ItemStack> drops, int droppedExp, int newExp, string deathMessage)
        : base(player, drops, droppedExp)
    {
        _deathMessage = deathMessage;
        _newExp = newExp;
        _newLevel = 0;
        _keepLevel = false;
        _keepInventory = false;
    }

    /// <summary>
    /// Returns the Entity involved in this event.
    /// </summary>
    /// <returns>Entity who is involved in this event.</returns>
    public new FourKitEntity.Player getEntity() => (FourKitEntity.Player)entity;

    /// <summary>
    /// Get the death message that will appear to everyone on the server.
    /// </summary>
    /// <returns>Message to appear to other players on the server.</returns>
    public string getDeathMessage() => _deathMessage;

    /// <summary>
    /// Set the death message that will appear to everyone on the server.
    /// </summary>
    /// <param name="deathMessage">Message to appear to other players on the server.</param>
    public void setDeathMessage(string deathMessage) => _deathMessage = deathMessage;

    /// <summary>
    /// Gets how much EXP the Player should have at respawn.
    /// This does not indicate how much EXP should be dropped, please see
    /// <see cref="EntityDeathEvent.getDroppedExp"/> for that.
    /// </summary>
    /// <returns>New EXP of the respawned player.</returns>
    public int getNewExp() => _newExp;

    /// <summary>
    /// Sets how much EXP the Player should have at respawn.
    /// This does not indicate how much EXP should be dropped, please see
    /// <see cref="EntityDeathEvent.setDroppedExp"/> for that.
    /// </summary>
    /// <param name="exp">New EXP of the respawned player.</param>
    public void setNewExp(int exp) => _newExp = exp;

    /// <summary>
    /// Gets the Level the Player should have at respawn.
    /// </summary>
    /// <returns>New Level of the respawned player.</returns>
    public int getNewLevel() => _newLevel;

    /// <summary>
    /// Sets the Level the Player should have at respawn.
    /// </summary>
    /// <param name="level">New Level of the respawned player.</param>
    public void setNewLevel(int level) => _newLevel = level;

    /// <summary>
    /// Gets if the Player should keep all EXP at respawn.
    /// This flag overrides other EXP settings.
    /// </summary>
    /// <returns><c>true</c> if Player should keep all pre-death exp.</returns>
    public bool getKeepLevel() => _keepLevel;

    /// <summary>
    /// Sets if the Player should keep all EXP at respawn.
    /// This overrides all other EXP settings.
    /// </summary>
    /// <param name="keepLevel"><c>true</c> to keep all current value levels.</param>
    public void setKeepLevel(bool keepLevel) => _keepLevel = keepLevel;

    /// <summary>
    /// Gets if the Player keeps inventory on death.
    /// </summary>
    /// <returns><c>true</c> if the player keeps inventory on death.</returns>
    public bool getKeepInventory() => _keepInventory;

    /// <summary>
    /// Sets if the Player keeps inventory on death.
    /// </summary>
    /// <param name="keepInventory"><c>true</c> to keep the inventory.</param>
    public void setKeepInventory(bool keepInventory) => _keepInventory = keepInventory;
}
