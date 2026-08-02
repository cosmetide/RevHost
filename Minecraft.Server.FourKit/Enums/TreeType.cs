using System;
using System.Collections.Generic;
using System.Text;

namespace Minecraft.Server.FourKit.Enums;

/// <summary>
/// Tree and organic structure types. 
/// </summary>
public enum TreeType {
    /// <summary>
    /// No tree type.
    /// </summary>
    None = 0,

    /// <summary>
    /// Redwood tree, shaped like a pine tree.
    /// </summary>
    SPRUCE = 1,

    /// <summary>
    /// Birch tree.
    /// </summary>
    BIRCH = 2,

    /// <summary>
    /// Standard jungle tree; 4 blocks wide and tall.
    /// </summary>
    JUNGLE = 3,

    /// <summary>
    /// Regular tree, extra tall with branches.
    /// </summary>
    BIG_OAK = 4,

    /// <summary>
    /// Regular tree, no branches.
    /// </summary>
    OAK = 5,

    /// <summary>
    /// Big brown mushroom; tall and umbrella-like.
    /// </summary>
    BROWN_MUSHROOM = 6,

    /// <summary>
    /// Big red mushroom; short and fat.
    /// </summary>
    RED_MUSHROOM = 7,
}
