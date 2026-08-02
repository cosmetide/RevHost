using Minecraft.Server.FourKit.Enums;
using System;
using System.Collections.Generic;
using System.Text;

namespace Minecraft.Server.FourKit.Event.World;

/// <summary>
/// Event that is called when an organic structure attempts to grow (Sapling -> Tree), (Mushroom -> Huge Mushroom), naturally or using bonemeal.
/// </summary>
public class StructureGrowEvent : WorldEvent, Cancellable
{
    internal Location _location;
    internal TreeType _treeType;
    internal bool _wasBonemeal;
    internal Minecraft.Server.FourKit.Entity.Player? _player;

    internal bool _cancelled;
    internal StructureGrowEvent(Location location, TreeType treeType, bool wasBonemeal, Minecraft.Server.FourKit.Entity.Player? player) : base(location.getWorld())
    {
        _location = location;
        _treeType = treeType;
        _wasBonemeal = wasBonemeal;
        _player = player;
    }


    /// <summary>
    /// Gets the species type (birch, normal, pine, red mushroom, brown mushroom).
    /// </summary>
    /// <returns>Structure species</returns>
    public TreeType getSpecies() => _treeType;


    /// <summary>
    /// Gets the location of the structure.
    /// </summary>
    /// <returns>Location of the structure</returns>
    public Location getLocation() => _location;


    /// <summary>
    /// Checks if structure was grown using bonemeal.
    /// </summary>
    /// <returns>True if the structure was grown using bonemeal.</returns>
    public bool isFromBonemeal() => _wasBonemeal;


    /// <summary>
    /// Gets the player that created the structure.
    /// </summary>
    /// <returns>Player that created the structure, null if was not created manually</returns>
    public Minecraft.Server.FourKit.Entity.Player? getPlayer() => _player;

    /// <inheritdoc/>
    public bool isCancelled() => _cancelled;

    /// <inheritdoc/>
    public void setCancelled(bool cancel) => _cancelled = cancel;
}
