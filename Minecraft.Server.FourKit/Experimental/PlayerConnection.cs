using Minecraft.Server.FourKit.Entity;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Minecraft.Server.FourKit.Experimental;

public class PlayerConnection
{
    private Player _player;

    internal PlayerConnection(Player player)
    {
        this._player = player;
    }

    /// <summary>
    /// Sends raw packet data directly to the client over the player's connection.
    /// The byte array must contain the complete packet including the packet ID as the first byte. The server automatically prepends the 4-byte big-endian size header before transmitting.
    /// </summary>
    /// <param name="data">The raw packet bytes to send, where <c>data[0]</c> is the packet ID.</param>
    public void send(byte[] data)
    {
        var gh = GCHandle.Alloc(data, GCHandleType.Pinned);
        try
        {
            NativeBridge.SendRaw?.Invoke(_player.getEntityId(), gh.AddrOfPinnedObject(), data.Length);
        }
        finally
        {
            gh.Free();
        }
    }
}
