namespace Minecraft.Server.FourKit.Event.Entity;

using FourKitEntity = Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Thrown whenever a LivingEntity dies.
/// </summary>
public class EntityDeathEvent : EntityEvent
{
    private readonly List<ItemStack> _drops;
    private int _droppedExp;

    internal EntityDeathEvent(FourKitEntity.LivingEntity entity, List<ItemStack> drops)
        : this(entity, drops, 0)
    {
    }

    internal EntityDeathEvent(FourKitEntity.LivingEntity what, List<ItemStack> drops, int droppedExp)
        : base(what)
    {
        _drops = drops;
        _droppedExp = droppedExp;
    }

    /// <summary>
    /// Returns the Entity involved in this event.
    /// </summary>
    /// <returns>Entity who is involved in this event.</returns>
    public new FourKitEntity.LivingEntity getEntity() => (FourKitEntity.LivingEntity)entity;

    /// <summary>
    /// Gets how much EXP should be dropped from this death.
    /// This does not indicate how much EXP should be taken from the entity
    /// in question, merely how much should be created after its death.
    /// </summary>
    /// <returns>Amount of EXP to drop.</returns>
    public int getDroppedExp() => _droppedExp;

    /// <summary>
    /// Sets how much EXP should be dropped from this death.
    /// This does not indicate how much EXP should be taken from the entity
    /// in question, merely how much should be created after its death.
    /// </summary>
    /// <param name="exp">Amount of EXP to drop.</param>
    public void setDroppedExp(int exp) => _droppedExp = exp;

    /// <summary>
    /// Gets all the items which will drop when the entity dies.
    /// </summary>
    /// <returns>Items to drop when the entity dies.</returns>
    public List<ItemStack> getDrops() => _drops;
}
