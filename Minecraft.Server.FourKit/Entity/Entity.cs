namespace Minecraft.Server.FourKit.Entity;

using Minecraft.Server.FourKit.Util;

/// <summary>
/// Represents a base entity in the world
/// </summary>
public class Entity
{
    private Location _location = new();
    private Guid _uniqueId = Guid.NewGuid();
    private float _fallDistance;
    private int _dimensionId;
    private int _entityId;
    private EntityType _entityType = EntityType.UNKNOWN;
    private bool _onGround;
    private double _velocityX, _velocityY, _velocityZ;

    /// <summary>
    /// Gets the entity's current position.
    /// </summary>
    /// <returns>a new copy of <see cref="Location"/> containing the position of this entity</returns>
    public Location getLocation() => _location;

    /// <summary>
    /// Returns a unique id for this entity
    /// </summary>
    /// <returns>Entity id</returns>
    public virtual int getEntityId() => _entityId;

    /// <summary>
    /// Get the type of the entity.
    /// </summary>
    /// <returns>The <see cref="EntityType"/> of this entity.</returns>
    public new virtual EntityType getType() => _entityType;
    public new virtual EntityType GetType() => _entityType;

    /// <summary>
    /// Returns a unique and persistent id for this entity. Note that this is not the standard UUID for players.
    /// </summary>
    /// <returns>A <see cref="Guid"/> unique to this entity.</returns>
    public Guid getUniqueId() => _uniqueId;

    /// <summary>
    /// Teleports this entity to the given location.
    /// This calls into the native server to perform the actual teleport.
    /// </summary>
    /// <param name="location">The destination location.</param>
    /// <returns><c>true</c> if the teleport was successful.</returns>
    public virtual bool teleport(Location location)
    {
        int targetDimId = location.LocationWorld?.getDimensionId() ?? _dimensionId;
        NativeBridge.TeleportEntity?.Invoke(getEntityId(), targetDimId, location.getX(), location.getY(), location.getZ());
        SetLocation(location);
        return true;
    }

    /// <summary>
    /// Sets the fall distance for this entity.
    /// </summary>
    /// <param name="distance">The fall distance value.</param>
    public void setFallDistance(float distance)
    {
        _fallDistance = distance;
        NativeBridge.SetFallDistance?.Invoke(getEntityId(), distance);
    }

    /// <summary>
    /// Returns the distance this entity has fallen.
    /// </summary>
    /// <returns>The current fall distance.</returns>
    public float getFallDistance() => _fallDistance;

    /// <summary>
    /// Gets the current world this entity resides in.
    /// </summary>
    /// <returns>World containing this entity.</returns>
    public World getWorld() => FourKit.getWorld(_dimensionId);

    /// <summary>
    /// Returns true if the entity is supported by a block. This value is a
    /// state updated by the server and is not recalculated unless the entity moves.
    /// </summary>
    /// <returns>True if entity is on ground.</returns>
    public bool isOnGround() => _onGround;

    /// <summary>
    /// Gets this entity's current velocity.
    /// </summary>
    /// <returns>Current travelling velocity of this entity.</returns>
    public Vector getVelocity() => new Vector(_velocityX, _velocityY, _velocityZ);

    /// <summary>
    /// Sets this entity's velocity.
    /// </summary>
    /// <param name="velocity">New velocity to travel with.</param>
    public void setVelocity(Vector velocity)
    {
        _velocityX = velocity.getX();
        _velocityY = velocity.getY();
        _velocityZ = velocity.getZ();
        NativeBridge.SetVelocity?.Invoke(getEntityId(), velocity.getX(), velocity.getY(), velocity.getZ());
    }

    /// <summary>
    /// Returns whether this entity is inside a vehicle.
    /// </summary>
    /// <returns><c>true</c> if the entity is in a vehicle.</returns>
    public bool isInsideVehicle()
    {
        return (NativeBridge.GetVehicleId?.Invoke(getEntityId()) ?? -1) >= 0;
    }

    /// <summary>
    /// Leave the current vehicle. If the entity is currently in a vehicle
    /// (and is removed from it), <c>true</c> will be returned, otherwise
    /// <c>false</c> will be returned.
    /// </summary>
    /// <returns><c>true</c> if the entity was in a vehicle.</returns>
    public bool leaveVehicle()
    {
        return NativeBridge.LeaveVehicle?.Invoke(getEntityId()) != 0;
    }

    /// <summary>
    /// Get the vehicle that this entity is inside. If there is no vehicle,
    /// <c>null</c> will be returned.
    /// </summary>
    /// <returns>The current vehicle, or <c>null</c>.</returns>
    public Entity? getVehicle()
    {
        int vehicleId = NativeBridge.GetVehicleId?.Invoke(getEntityId()) ?? -1;
        if (vehicleId < 0) return null;
        return FourKit.GetEntityByEntityId(vehicleId);
    }

    /// <summary>
    /// Eject any passenger.
    /// </summary>
    /// <returns><c>true</c> if there was a passenger.</returns>
    public bool eject()
    {
        return NativeBridge.Eject?.Invoke(getEntityId()) != 0;
    }

    /// <summary>
    /// Gets the primary passenger of a vehicle. For vehicles that could
    /// have multiple passengers, this will only return the primary passenger.
    /// </summary>
    /// <returns>The passenger entity, or <c>null</c>.</returns>
    public Entity? getPassenger()
    {
        int passengerId = NativeBridge.GetPassengerId?.Invoke(getEntityId()) ?? -1;
        if (passengerId < 0) return null;
        return FourKit.GetEntityByEntityId(passengerId);
    }

    /// <summary>
    /// Set the passenger of a vehicle.
    /// </summary>
    /// <param name="passenger">The new passenger.</param>
    /// <returns><c>false</c> if it could not be done for whatever reason.</returns>
    public bool setPassenger(Entity passenger)
    {
        if (passenger == null || NativeBridge.SetPassenger == null) return false;
        return NativeBridge.SetPassenger(getEntityId(), passenger.getEntityId()) != 0;
    }

    // INTERNAL
    internal void SetLocation(Location location)
    {
        _location = location;
    }

    internal void SetFallDistanceInternal(float distance) => _fallDistance = distance;

    internal void SetUniqueId(Guid id)
    {
        _uniqueId = id;
    }

    internal void SetDimensionInternal(int dimensionId) => _dimensionId = dimensionId;
    internal void SetEntityIdInternal(int entityId) => _entityId = entityId;
    internal void SetEntityTypeInternal(EntityType entityType) => _entityType = entityType;
    internal void SetOnGroundInternal(bool onGround) => _onGround = onGround;
    internal void SetVelocityInternal(double x, double y, double z)
    {
        _velocityX = x;
        _velocityY = y;
        _velocityZ = z;
    }
}
