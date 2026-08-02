@page sending-packets Sending Packets

This page covers how to manually send raw packets to clients using the \ref Minecraft.Server.FourKit.Experimental.PlayerConnection "PlayerConnection" API. This is an **experimental** feature for advanced use cases where you need to send data that FourKit doesnt yet wrap in its API.

> **This API is experimental and may change. You are responsible for constructing valid packets!!! malformed data can crash or disconnect the client.**

Also please keep in mind, some of this info may not be accurate! Feel free to improve this by contributing on the Github.

---

@section packet_overview Overview

Every Minecraft packet on the wire looks like this:

| Field | Size | Description |
|-------|------|-------------|
| Size | 4 bytes (big-endian) | Total length of the remaining data (packet ID + payload). **Written automatically by the server.. you do NOT include it.** |
| Packet ID | 1 byte | Which packet this is. Only the low byte matters on the wire even though IDs are `int` in source. |
| Payload | variable | The rest of the packet data, format depends on the packet ID. |

When you call `PlayerConnection.send(byte[] data)`, the server prepends the 4-byte big-endian size header for you. Your byte array should start with the packet ID byte followed by the payload.

---

@section wire_data_types Wire Data Types

All multi-byte values are **big-endian** (most significant byte first).

| Type | Size | Description |
|------|------|-------------|
| `byte` | 1 | Unsigned 8-bit integer (0-255). |
| `bool` | 1 | 0 = false, non-zero = true. |
| `short` | 2 | Signed 16-bit integer. |
| `int` | 4 | Signed 32-bit integer. |
| `long` | 8 | Signed 64-bit integer. |
| `float` | 4 | IEEE 754 single-precision float. |
| `utf` | 2 + n | Modified UTF-8 string: `short` byte-length prefix followed by that many bytes of data. See \ref string_encoding "String Encoding". |
| `item` | variable | Item data (item ID, count, damage). See \ref item_data "Item Data". |
| `metadata` | variable | Entity metadata (SynchedEntityData). See \ref metadata_encoding "Metadata Encoding". |

---

@section string_encoding String Encoding

Strings use modified UTF-8 encoding:

1. A `short` (2 bytes, big-endian) containing the byte length of the string data.
2. That many bytes of modified-UTF8 characters.

For most ASCII text (chat messages, names, etc.) this is identical to standard UTF-8. A helper to write a string:

```csharp
static int WriteUTF(byte[] buffer, int offset, string text)
{
    byte[] utf8 = System.Text.Encoding.UTF8.GetBytes(text);
    WriteShort(buffer, offset, (short)utf8.Length);
    Buffer.BlockCopy(utf8, 0, buffer, offset + 2, utf8.Length);
    return 2 + utf8.Length; // total bytes written
}
```

---

@section item_data Item Data

Several container packets include serialized item data. The wire format for a single item slot is:

| Field | Type | Description |
|-------|------|-------------|
| id | `short` | Item ID. `-1` means the slot is empty (no further fields follow). |
| count | `byte` | Stack size. |
| damage | `short` | Damage/metadata value. |

If the item ID is `-1`, only the `short` is written (2 bytes for an empty slot). Otherwise all three fields are written (5 bytes).

---

@section metadata_encoding Metadata Encoding (SynchedEntityData)

Entity metadata is used in packets like AddMobPacket (24), AddPlayerPacket (20), and SetEntityDataPacket (40). It's a list of typed key-value entries terminated by `0x7F`.

Each entry starts with a 1-byte header:
- **Bits 5-7** (mask `0xE0`): Type ID, shifted right by 5.
- **Bits 0-4** (mask `0x1F`): Data slot ID (max 31).

So the header byte is `(type << 5) | (id & 0x1F)`.

The value immediately follows, sized based on the type:

| Type ID | Name | Value Size |
|---------|------|------------|
| 0 | Byte | 1 byte |
| 1 | Short | 2 bytes |
| 2 | Int | 4 bytes |
| 3 | Float | 4 bytes |
| 4 | String | `utf` (2-byte length prefix + string data) |
| 5 | ItemInstance | `item` (see \ref item_data "Item Data") |

After all entries, write `0x7F` as the EOF marker.

@subsubsection metadata_data_ids Common Data Slot IDs

These are the data slot IDs used by the entity class hierarchy. When constructing metadata, define these in order and use the correct type.

**Entity (base):**
| ID | Type | Name | Description |
|----|------|------|-------------|
| 0 | Byte | DATA_SHARED_FLAGS | Bitfield: bit 0=on fire, 1=sneaking, 3=sprinting, 4=using item, 5=invisible, 6=idle anim |
| 1 | Short | DATA_AIR_SUPPLY | Air supply (default 300, max 300) |

**LivingEntity:**
| ID | Type | Name | Description |
|----|------|------|-------------|
| 6 | Float | DATA_HEALTH | Health value (default 1.0) |
| 7 | Int | DATA_EFFECT_COLOR | Potion effect color (default 0) |
| 8 | Byte | DATA_EFFECT_AMBIENCE | Potion effect ambience (default 0) |
| 9 | Byte | DATA_ARROW_COUNT | Number of arrows stuck in entity (default 0) |

**Mob:**
| ID | Type | Name | Description |
|----|------|------|-------------|
| 10 | String | DATA_CUSTOM_NAME | Custom name tag (default "") |
| 11 | Byte | DATA_CUSTOM_NAME_VISIBLE | Show name tag (0 or 1, default 0) |

For example, to write metadata for a basic mob with a custom name:

```csharp
// helper to write a metadata entry header
static void WriteMetaHeader(byte[] buffer, int offset, int type, int id)
{
    buffer[offset] = (byte)((type << 5) | (id & 0x1F));
}

// build metadata for a mob with a visible custom name "booty"
// enity base: flags=0, air=300
// livingentity: health=20.0, effectColor=0, effectAmbience=0, arrowCount=0
// mob: customName="booty", customNameVisible=1

byte[] nameBytes = System.Text.Encoding.UTF8.GetBytes("booty");
int metaSize = (1+1) + (1+2) + (1+4) + (1+4) + (1+1) + (1+1)
             + (1 + 2 + nameBytes.Length) + (1+1) + 1; // +1 for EOF
byte[] meta = new byte[metaSize];
int pos = 0;

// Entity
WriteMetaHeader(meta, pos, 0, 0); pos++; meta[pos++] = 0; // flags = 0
WriteMetaHeader(meta, pos, 1, 1); pos++; WriteShort(meta, pos, 300); pos += 2; // air supply

// LivingEntity
WriteMetaHeader(meta, pos, 3, 6); pos++; WriteFloat(meta, pos, 20.0f); pos += 4; // health
WriteMetaHeader(meta, pos, 2, 7); pos++; WriteInt(meta, pos, 0); pos += 4; // effect color
WriteMetaHeader(meta, pos, 0, 8); pos++; meta[pos++] = 0; // effect ambience
WriteMetaHeader(meta, pos, 0, 9); pos++; meta[pos++] = 0; // arrow count

// Mob
WriteMetaHeader(meta, pos, 4, 10); pos++;
pos += WriteUTF(meta, pos, "booty"); // custom name
WriteMetaHeader(meta, pos, 0, 11); pos++; meta[pos++] = 1; // name visible

meta[pos] = 0x7F; // EOF marker
```

---

@section writing_data Writing Packet Data

Helper methods for writing big-endian values into a `byte[]` buffer:

```csharp
static void WriteByte(byte[] buffer, int offset, int value)
{
    buffer[offset] = (byte)(value & 0xFF);
}

static void WriteBool(byte[] buffer, int offset, bool value)
{
    buffer[offset] = (byte)(value ? 1 : 0);
}

static void WriteShort(byte[] buffer, int offset, short value)
{
    buffer[offset] = (byte)((value >> 8) & 0xFF);
    buffer[offset + 1] = (byte)(value & 0xFF);
}

static void WriteInt(byte[] buffer, int offset, int value)
{
    buffer[offset] = (byte)((value >> 24) & 0xFF);
    buffer[offset + 1] = (byte)((value >> 16) & 0xFF);
    buffer[offset + 2] = (byte)((value >> 8) & 0xFF);
    buffer[offset + 3] = (byte)(value & 0xFF);
}

static void WriteFloat(byte[] buffer, int offset, float value)
{
    byte[] bytes = BitConverter.GetBytes(value);
    if (BitConverter.IsLittleEndian)
        Array.Reverse(bytes);
    Buffer.BlockCopy(bytes, 0, buffer, offset, 4);
}

static void WriteLong(byte[] buffer, int offset, long value)
{
    buffer[offset] = (byte)((value >> 56) & 0xFF);
    buffer[offset + 1] = (byte)((value >> 48) & 0xFF);
    buffer[offset + 2] = (byte)((value >> 40) & 0xFF);
    buffer[offset + 3] = (byte)((value >> 32) & 0xFF);
    buffer[offset + 4] = (byte)((value >> 24) & 0xFF);
    buffer[offset + 5] = (byte)((value >> 16) & 0xFF);
    buffer[offset + 6] = (byte)((value >> 8) & 0xFF);
    buffer[offset + 7] = (byte)(value & 0xFF);
}

static void WriteEmptyItem(byte[] buffer, int offset)
{
    WriteShort(buffer, offset, (short)-1);
}

static void WriteItem(byte[] buffer, int offset, short id, byte count, short damage)
{
    WriteShort(buffer, offset, id);
    buffer[offset + 2] = count;
    WriteShort(buffer, offset + 3, damage);
}
```

---

@section precision_scaling Precision Scaling

Some fields use fixed-point encoding integers on the wire representing floats/doubles with a multiplier applied:

| Data Type | Multiplier | Wire Type | Description |
|-----------|------------|-----------|-------------|
| Entity position | x 32 | `int` | `(int)(position * 32)`. 1/32 block precision. |
| Sound position | x 8 | `int` | `(int)(position * 8)`. 1/8 block precision. |
| Rotation | x 256 / 360 | `byte` | `(byte)(angle * 256.0 / 360.0)`. |
| Velocity | x 8000 | `short` | `(short)(velocity * 8000.0)`, clamped to +/-3.9 blocks/tick before encoding. |

```csharp
int wireX = (int)(player.getLocation().getX() * 32);
int wireY = (int)(player.getLocation().getY() * 32);
int wireZ = (int)(player.getLocation().getZ() * 32);

byte wireYaw = (byte)(player.getLocation().getYaw() * 256.0 / 360.0);
byte wirePitch = (byte)(player.getLocation().getPitch() * 256.0 / 360.0);
```

---

@section example_sound Example: Playing a Sound Effect

Sends a LevelSoundPacket (ID 62) to play a ghast scream at the player's location whenever they chat.

```csharp
[EventHandler]
public void onPlayerChat(PlayerChatEvent e)
{
    // LevelSoundPacket layout:
    //   [0]      byte   packet ID (62)
    //   [1..4]   int    sound type
    //   [5..8]   int    x * 8
    //   [9..12]  int    y * 8
    //   [13..16] int    z * 8
    //   [17..20] float  volume
    //   [21..24] float  pitch
    byte[] buffer = new byte[25];
    buffer[0] = (byte)62; // packet id

    WriteInt(buffer, 1, (int)19);  // eSoundType_MOB_GHAST_SCREAM (Sound.GHAST_SCREAM)

    WriteInt(buffer, 5,  (int)(e.getPlayer().getLocation().getX() * 8));
    WriteInt(buffer, 9,  (int)(e.getPlayer().getLocation().getY() * 8));
    WriteInt(buffer, 13, (int)(e.getPlayer().getLocation().getZ() * 8));

    WriteFloat(buffer, 17, 10);  // volume
    WriteFloat(buffer, 21, 1);   // pitch

    e.getPlayer().getConnection().send(buffer);
}
```

Sound type IDs correspond to the \ref Minecraft.Server.FourKit.Sound "Sound" enum. Cast any `Sound` value to `int` to get the wire value, for example `(int)Sound.GHAST_SCREAM` is `19`.

---

@section example_gamee Example: Changing the Game Mode

GameEventPacket (ID 70) notifies the client of game state changes.

```csharp
[EventHandler]
public void onJoin(PlayerJoinEvent e)
{
    // layout:
    //   [0]  byte  packet ID (70)
    //   [1]  byte  event type
    //   [2]  byte  parameter
    byte[] buffer = new byte[3];
    buffer[0] = (byte)70;  // packet id
    buffer[1] = (byte)3;   // CHANGE_GAME_MODE
    buffer[2] = (byte)1;   // 0 = Survival, 1 = Creative, 2 = Adventure
    e.getPlayer().getConnection().send(buffer);
}
```

---

@section example_entity_teleport Example: Teleporting an Entity (Client-Side)

TeleportEntityPacket (ID 34) moves an entity to an absolute position on the client.

```csharp
// Teleport entity 42 to (100.5, 64.0, -200.25)
byte[] buffer = new byte[17]
buffer[0] = (byte)34; // packet id

WriteShort(buffer, 1, (short)42); // entity id
WriteInt(buffer, 3,  (int)(100.5 * 32)); // x * 32
WriteInt(buffer, 7,  (int)(64.0 * 32)); // y * 32
WriteInt(buffer, 11, (int)(-200.25 * 32)); // z * 32
buffer[15] = (byte)(90.0 * 256.0 / 360.0); // yRot
buffer[16] = (byte)(0.0 * 256.0 / 360.0); // xRot

player.getConnection().send(buffer);
```

---

@section example_set_time Example: Setting the World Time

SetTimePacket (ID 4) updates the client's world time.

```csharp
// set time to noon (6000 ticks)
byte[] buffer = new byte[17]
buffer[0] = (byte)4; // packet id

WriteLong(buffer, 1, 6000L); // game time (total ticks elapsed)
WriteLong(buffer, 9, 6000L); // day time (time of day, 0-24000)

player.getConnection().send(buffer);
```

---

@section example_health Example: Updating Health Display

SetHealthPacket (ID 8) updates the client's health, food, and saturation display.

```csharp
// set health to 20 (full), food to 20 (full), saturation to 5.0
byte[] buffer = new byte[12];
buffer[0] = (byte)8; // packet id

WriteFloat(buffer, 1, 20.0f); // health
WriteShort(buffer, 5, (short)20); // food
WriteFloat(buffer, 7, 5.0f); // saturation
buffer[11] = (byte)0; // damage source (0 = unknown)

player.getConnection().send(buffer);
```

---

@section example_abilities Example: Setting Player Abilities

PlayerAbilitiesPacket (ID 202) updates the player's ability flags and speeds.

```csharp
// enable flying for the player
byte[] buffer = new byte[10];
buffer[0] = (byte)202; // packet id

// 0x01=invulnerable, 0x02=flying, 0x04=canFly, 0x08=instabuild (creative)
buffer[1] = (byte)(0x02 | 0x04); // flying + canFly

WriteFloat(buffer, 2, 0.05f); // fly speed (default 0.05)
WriteFloat(buffer, 6, 0.1f); // walk speed (default 0.1)

player.getConnection().send(buffer);
```

---

@section example_explosion Example: Creating an Explosion Effect

ExplodePacket (ID 60) creates a client-side explosion with optional block destruction and knockback.

```csharp
// create an explosion at (100.0, 64.0, 200.0) with radius 4.0 and no destroyed blocks
byte[] buffer = new byte[46];
buffer[0] = (byte)60; // packet id

buffer[1] = (byte)0; // knockbackOnly = false (full explosion with position data)

// position (double precision, 8 bytes each)
byte[] xBytes = BitConverter.GetBytes(100.0);
byte[] yBytes = BitConverter.GetBytes(64.0);
byte[] zBytes = BitConverter.GetBytes(200.0);
Buffer.BlockCopy(xBytes, 0, buffer, 2, 8);
Buffer.BlockCopy(yBytes, 0, buffer, 10, 8);
Buffer.BlockCopy(zBytes, 0, buffer, 18, 8);

WriteFloat(buffer, 26, 4.0f); // radius
WriteInt(buffer, 30, 0); // destroyed block count (0 = no blocks)

// knockback velocity applied to player
WriteFloat(buffer, 34, 0.0f); // knockback X
WriteFloat(buffer, 38, 0.5f); // knockback Y (push player up)
WriteFloat(buffer, 42, 0.0f); // knockback Z

player.getConnection().send(buffer);
```

---

@section example_xp Example: Setting the XP Bar

SetExperiencePacket (ID 43) updates the XP bar.

```csharp
// Set XP bar to 50% progress, level 10, total 300 XP
byte[] buffer = new byte[1 + 4 + 2 + 2]; // 9 bytes
buffer[0] = (byte)43; // packet id

WriteFloat(buffer, 1, 0.5f);        // bar progress (0.0 to 1.0)
WriteShort(buffer, 5, (short)10);   // level
WriteShort(buffer, 7, (short)300);  // total XP points

player.getConnection().send(buffer);
```

---

@section packet_reference Complete packet reference

All server-to-client packet layouts. The packet ID byte is always `buffer[0]` and is not listed in the field tables below. only the payload after the ID is shown.

@subsection packet_ref_general General / Connection Packets

@subsubsection pkt_0 Packet 0 - KeepAlivePacket

Connection keepalive. The client echoes this back.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | id | Keepalive ID. The client should respond with the same value. |

**Total payload: 4 bytes.**

@subsubsection pkt_3 Packet 3 - ChatPacket

Send a chat/system message to the client.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `short` | messageType | Message type enum value. |
| 2 | `short` | packedCounts | Packed field: high nibble = string arg count, low nibble = int arg count. Computed as `(stringCount << 4) \| intCount`. |
| 4 | `utf[]` | stringArgs | Variable number of `utf` strings (the message text, source name, item name, etc). |
| ... | `int[]` | intArgs | Variable number of `int` values (for example source entity type). |

**Total payload: variable.** For a simple chat message, `messageType` = 0, one string arg (the message), zero int args. The packed counts would be `(1 << 4) | 0` = `0x0010`.

@subsubsection pkt_255 Packet 255 - DisconnectPacket

Disconnect the client.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | reason | Disconnect reason code. |

**Total payload: 4 bytes.**

@subsection packet_ref_world World & Environment Packets

@subsubsection pkt_4 Packet 4 - SetTimePacket

Set world time.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `long` | gameTime | Total game time in ticks (monotonically increasing). |
| 8 | `long` | dayTime | Time of day in ticks (0-24000 range). |

**Total payload: 16 bytes.**

@subsubsection pkt_6 Packet 6 - SetSpawnPositionPacket

Set the world spawn point.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | x | Spawn X coordinate. |
| 4 | `int` | y | Spawn Y coordinate. |
| 8 | `int` | z | Spawn Z coordinate. |

**Total payload: 12 bytes.**

@subsubsection pkt_9 Packet 9 - RespawnPacket

Sent on respawn or dimension change.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `byte` | dimension | Dimension ID (0=Overworld, -1=Nether, 1=End). |
| 1 | `byte` | gameType | Game mode ID (0=Survival, 1=Creative, 2=Adventure). |
| 2 | `short` | mapHeight | World height. |
| 4 | `utf` | levelType | Level type name (for example "DEFAULT", "FLAT"). |
| ... | `long` | mapSeed | World seed. |
| ... | `byte` | difficulty | Difficulty (0=Peaceful, 1=Easy, 2=Normal, 3=Hard). |
| ... | `bool` | newSeaLevel | Whether the new sea level is active. |
| ... | `short` | newEntityId | The player's new entity ID. |
| ... | `short` | xzSize | World XZ size. |
| ... | `byte` | hellScale | Nether scale factor. |

**Total payload: variable** (depends on level type string length).

@subsubsection pkt_53 Packet 53 - TileUpdatePacket

Update a single block.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | x | Block X coordinate. |
| 4 | `byte` | y | Block Y coordinate (0-255). |
| 5 | `int` | z | Block Z coordinate. |
| 9 | `short` | block | Block type ID. |
| 11 | `byte` | dataLevel | Block data/metadata value. |

**Total payload: 12 bytes.**

@subsubsection pkt_54 Packet 54 - TileEventPacket

Trigger a block action (note blocks, pistons, chests).

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | x | Block X coordinate. |
| 4 | `short` | y | Block Y coordinate. |
| 6 | `int` | z | Block Z coordinate. |
| 10 | `byte` | b0 | Action parameter 1 (depends on block type). |
| 11 | `byte` | b1 | Action parameter 2 (depends on block type). |
| 12 | `short` | tile | Block type ID. |

**Total payload: 14 bytes.**

@subsubsection pkt_55 Packet 55 - TileDestructionPacket

Show a block breaking animation (crack overlay).

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | id | Breaker entity ID. |
| 4 | `int` | x | Block X coordinate. |
| 8 | `int` | y | Block Y coordinate. |
| 12 | `int` | z | Block Z coordinate. |
| 16 | `byte` | state | Destroy stage (0-9). Any other value removes the overlay. |

**Total payload: 17 bytes.**

@subsubsection pkt_60 Packet 60 - ExplodePacket

Explosion with optional block destruction and knockback.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `bool` | knockbackOnly | If true, only knockback fields follow (no position/radius/blocks). |

If `knockbackOnly` is **false** (typical explosion):

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 1 | `double` | x | Explosion center X. |
| 9 | `double` | y | Explosion center Y. |
| 17 | `double` | z | Explosion center Z. |
| 25 | `float` | radius | Explosion radius. |
| 29 | `int` | count | Number of destroyed block offsets. |
| 33 | `byte[count*3]` | offsets | For each block: 3 signed bytes (dx, dy, dz) relative to the center. |
| ... | `float` | knockbackX | Player knockback velocity X. |
| ... | `float` | knockbackY | Player knockback velocity Y. |
| ... | `float` | knockbackZ | Player knockback velocity Z. |

If `knockbackOnly` is **true** (just apply knockback, no visual explosion):

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 1 | `float` | knockbackX | Player knockback velocity X. |
| 5 | `float` | knockbackY | Player knockback velocity Y. |
| 9 | `float` | knockbackZ | Player knockback velocity Z. |

**Total payload: variable.**

@subsubsection pkt_61 Packet 61 - LevelEventPacket

World event (sounds, particles, door effects, etc).

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | type | Event type ID (for example 1000=click sound, 1005=bow sound, 2000=smoke, 2001=break block). |
| 4 | `int` | x | Block X coordinate. |
| 8 | `byte` | y | Block Y coordinate. |
| 9 | `int` | z | Block Z coordinate. |
| 13 | `int` | data | Event-specific data (for example block ID for break effect, direction for smoke). |
| 17 | `bool` | globalEvent | If true, event is global (all players hear it regardless of distance). |

**Total payload: 18 bytes.**

@subsubsection pkt_62 Packet 62 - LevelSoundPacket

Play a sound at a position. Positions use x8 scaling.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | soundId | Sound type ID. Maps to the \ref Minecraft.Server.FourKit.Sound "Sound" enum. |
| 4 | `int` | x | X coordinate * 8. |
| 8 | `int` | y | Y coordinate * 8. |
| 12 | `int` | z | Z coordinate * 8. |
| 16 | `float` | volume | Sound volume (1.0 = normal). |
| 20 | `float` | pitch | Sound pitch (1.0 = normal). |

**Total payload: 24 bytes.**

@subsubsection pkt_63 Packet 63 - LevelParticlesPacket

Spawn particles at a position.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `utf` | name | Particle name string (for example "flame", "smoke", "heart"). |
| ... | `float` | x | Center X coordinate. |
| ... | `float` | y | Center Y coordinate. |
| ... | `float` | z | Center Z coordinate. |
| ... | `float` | xDist | Random X offset range. |
| ... | `float` | yDist | Random Y offset range. |
| ... | `float` | zDist | Random Z offset range. |
| ... | `float` | maxSpeed | Maximum particle speed. |
| ... | `int` | count | Number of particles to spawn. |

**Total payload: variable** (depends on particle name string length) **+ 32 bytes** for the fixed fields.

@subsubsection pkt_70 Packet 70 - GameEventPacket

Game state change notification.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `byte` | event | Event type. |
| 1 | `byte` | param | Event parameter. |

Event types:
| Value | Name | Parameter |
|-------|------|-----------|
| 0 | No Bed | (unused) |
| 1 | Start Rain | (unused) |
| 2 | Stop Rain | (unused) |
| 3 | Change Game Mode | 0=Survival, 1=Creative, 2=Adventure |
| 4 | Win Game | 0=show credits, 1=just respawn |

**Total payload: 2 bytes.**

@subsubsection pkt_130 Packet 130 - SignUpdatePacket

Update sign text.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | x | Sign X coordinate. |
| 4 | `short` | y | Sign Y coordinate. |
| 6 | `int` | z | Sign Z coordinate. |
| 10 | `bool` | verified | Whether the sign text has been verified. |
| 11 | `bool` | censored | Whether the sign text has been censored. |
| 12 | `utf` | line1 | First line of text. |
| ... | `utf` | line2 | Second line of text. |
| ... | `utf` | line3 | Third line of text. |
| ... | `utf` | line4 | Fourth line of text. |

**Total payload: variable** (12 bytes fixed + 4 * `utf` strings).

@subsection packet_ref_entity Entity Packets

@subsubsection pkt_8 Packet 8 - SetHealthPacket

Update health, food, and saturation.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `float` | health | Current health (0.0-20.0, 20 = full). |
| 4 | `short` | food | Food level (0-20). |
| 6 | `float` | saturation | Food saturation level. |
| 10 | `byte` | damageSource | Damage source type (for telemetry). |

**Total payload: 11 bytes.**

@subsubsection pkt_18 Packet 18 - AnimatePacket

Play an entity animation.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | entityId | Entity ID. |
| 4 | `byte` | action | Animation type (1=swing arm, 2=damage, 3=leave bed, 104=crouch, 105=uncrouch). |

**Total payload: 5 bytes.**

@subsubsection pkt_20 Packet 20 - AddPlayerPacket

Spawn a named player entity.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | entityId | Entity ID. |
| 4 | `utf` | name | Player name. |
| ... | `int` | x | X coordinate * 32 (fixed-point). |
| ... | `int` | y | Y coordinate * 32 (fixed-point). |
| ... | `int` | z | Z coordinate * 32 (fixed-point). |
| ... | `byte` | yRot | Yaw (angle * 256 / 360). |
| ... | `byte` | xRot | Pitch (angle * 256 / 360). |
| ... | `byte` | yHeadRot | Head yaw (angle * 256 / 360). |
| ... | `short` | carriedItem | Item ID of held item. |
| ... | `playerUID` | xuid | Player XUID (8 bytes). |
| ... | `playerUID` | onlineXuid | Online XUID for splitscreen guests (8 bytes). |
| ... | `byte` | playerIndex | Local player index. |
| ... | `int` | skinId | Custom skin ID. |
| ... | `int` | capeId | Custom cape ID. |
| ... | `int` | gamePrivileges | Player game privileges bitfield. |
| ... | `metadata` | entityData | Entity metadata (SynchedEntityData). |

**Total payload: variable** (includes entity metadata).

@subsubsection pkt_23 Packet 23 - AddEntityPacket

Spawn a non-mob entity (minecart, arrow, falling block, etc).

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `short` | entityId | Entity ID. |
| 2 | `byte` | type | Entity type ID (for example 1=boat, 10=minecart, 50=TNT, 60=arrow, 70=falling block). |
| 3 | `int` | x | X coordinate * 32 (fixed-point). |
| 7 | `int` | y | Y coordinate * 32 (fixed-point). |
| 11 | `int` | z | Z coordinate * 32 (fixed-point). |
| 15 | `byte` | yRot | Yaw (angle * 256 / 360). |
| 16 | `byte` | xRot | Pitch (angle * 256 / 360). |
| 17 | `int` | data | Entity-specific data (for example block ID for falling blocks, owner entity ID for projectiles). |

If `data` is non-zero, three additional velocity fields follow:

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 21 | `short` | xVel | X velocity * 8000. |
| 23 | `short` | yVel | Y velocity * 8000. |
| 25 | `short` | zVel | Z velocity * 8000. |

**Total payload: 21 bytes** (no velocity) or **27 bytes** (with velocity).

@subsubsection pkt_24 Packet 24 - AddMobPacket

Spawn a mob.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `short` | entityId | Entity ID. |
| 2 | `byte` | mobType | Mob type ID (for example 50=Creeper, 51=Skeleton, 52=Spider, 54=Zombie, 90=Pig, 91=Sheep). |
| 3 | `int` | x | X coordinate * 32 (fixed-point). |
| 7 | `int` | y | Y coordinate * 32 (fixed-point). |
| 11 | `int` | z | Z coordinate * 32 (fixed-point). |
| 15 | `byte` | yRot | Yaw (angle * 256 / 360). |
| 16 | `byte` | xRot | Pitch (angle * 256 / 360). |
| 17 | `byte` | yHeadRot | Head yaw (angle * 256 / 360). |
| 18 | `short` | xVel | X velocity * 8000. |
| 20 | `short` | yVel | Y velocity * 8000. |
| 22 | `short` | zVel | Z velocity * 8000. |
| 24 | `metadata` | entityData | Entity metadata (SynchedEntityData). |

**Total payload: 24 bytes + variable metadata.**

@subsubsection pkt_26 Packet 26 - AddExperienceOrbPacket

Spawn an XP orb.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | entityId | Entity ID. |
| 4 | `int` | x | X coordinate * 32 (fixed-point). |
| 8 | `int` | y | Y coordinate * 32 (fixed-point). |
| 12 | `int` | z | Z coordinate * 32 (fixed-point). |
| 16 | `short` | value | XP value of the orb. |

**Total payload: 18 bytes.**

@subsubsection pkt_28 Packet 28 - SetEntityMotionPacket

Set entity velocity. Has two encoding modes based on a flag bit in the entity ID field.

The first field is a `short` combining the entity ID and a flag:
- **Bits 0-10** (mask `0x07FF`): Entity ID (max 2047).
- **Bit 11** (mask `0x0800`): If set, velocity uses 3 bytes (lower precision). If clear, 3 shorts (full precision).

**Full precision mode** (flag clear):

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `short` | idAndFlag | Entity ID (low 11 bits), flag=0. |
| 2 | `short` | xVel | X velocity * 8000. |
| 4 | `short` | yVel | Y velocity * 8000. |
| 6 | `short` | zVel | Z velocity * 8000. |

**Total payload: 8 bytes.**

**Compact mode** (flag set, bit 11 = 1):

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `short` | idAndFlag | Entity ID (low 11 bits) OR `0x0800`. |
| 2 | `byte` | xVel | X velocity / 16 (sign-extended, then multiplied by 16 on read). |
| 3 | `byte` | yVel | Y velocity / 16. |
| 4 | `byte` | zVel | Z velocity / 16. |

**Total payload: 5 bytes.** The server automatically picks compact mode when velocity values fit.

@subsubsection pkt_29 Packet 29 - RemoveEntitiesPacket

Despawn one or more entities.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `byte` | count | Number of entities to remove. |
| 1 | `int[count]` | entityIds | Array of entity IDs (4 bytes each). |

**Total payload: 1 + (count * 4) bytes.**

@subsubsection pkt_30_33 Packets 30-33 - MoveEntityPacket

Relative entity movement/rotation updates. Four sub-types:

**Packet 30 - MoveEntityPacket** (base, no movement): `short` entityId only. **Payload: 2 bytes.**

**Packet 31 - MoveEntityPacket.Pos** (position only):

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `short` | entityId | Entity ID. |
| 2 | `byte` | dx | X delta (signed, in 1/32 block units). |
| 3 | `byte` | dy | Y delta. |
| 4 | `byte` | dz | Z delta. |

**Payload: 5 bytes.**

**Packet 32 - MoveEntityPacket.Rot** (rotation only):

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `short` | entityId | Entity ID. |
| 2 | `byte` | yRot | New yaw (angle * 256 / 360). |
| 3 | `byte` | xRot | New pitch (angle * 256 / 360). |

**Payload: 4 bytes.**

**Packet 33 - MoveEntityPacket.PosRot** (position + rotation):

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `short` | entityId | Entity ID. |
| 2 | `byte` | dx | X delta. |
| 3 | `byte` | dy | Y delta. |
| 4 | `byte` | dz | Z delta. |
| 5 | `byte` | yRot | New yaw. |
| 6 | `byte` | xRot | New pitch. |

**Payload: 7 bytes.**

@subsubsection pkt_34 Packet 34 - TeleportEntityPacket

Teleport an entity to an absolute position.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `short` | entityId | Entity ID. |
| 2 | `int` | x | X coordinate * 32 (fixed-point). |
| 6 | `int` | y | Y coordinate * 32 (fixed-point). |
| 10 | `int` | z | Z coordinate * 32 (fixed-point). |
| 14 | `byte` | yRot | Yaw (angle * 256 / 360). |
| 15 | `byte` | xRot | Pitch (angle * 256 / 360). |

**Total payload: 16 bytes.**

@subsubsection pkt_35 Packet 35 - RotateHeadPacket

Update an entity's head rotation.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | entityId | Entity ID. |
| 4 | `byte` | yHeadRot | Head yaw (angle * 256 / 360). |

**Total payload: 5 bytes.**

@subsubsection pkt_38 Packet 38 - EntityEventPacket

Trigger an entity event (hurt, death, eating, etc).

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | entityId | Entity ID. |
| 4 | `byte` | eventId | Event type (2=hurt, 3=death, 9=eating finished). |

**Total payload: 5 bytes.**

@subsubsection pkt_39 Packet 39 - SetEntityLinkPacket

Attach or detach entities (leash, riding).

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | sourceId | Entity being attached (the rider/leashed entity). |
| 4 | `int` | destId | Entity being attached to (the vehicle/fence). `-1` to detach. |
| 8 | `byte` | type | Link type. |

**Total payload: 9 bytes.**

@subsubsection pkt_40 Packet 40 - SetEntityDataPacket

Update entity metadata. See \ref metadata_encoding "Metadata Encoding" for how to construct the metadata blob.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | entityId | Entity ID. |
| 4 | `metadata` | data | Packed entity metadata entries. |

**Total payload: 4 bytes + variable metadata.**

@subsubsection pkt_41 Packet 41 - UpdateMobEffectPacket

Apply or update a potion effect.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | entityId | Entity ID. |
| 4 | `byte` | effectId | Effect ID (1=speed, 2=slowness, 3=haste, 4=mining fatigue, 5=strength, ...). |
| 5 | `byte` | amplifier | Effect level (0 = level I, 1 = level II, etc). |
| 6 | `short` | duration | Duration in ticks. |

**Total payload: 8 bytes.**

@subsubsection pkt_42 Packet 42 - RemoveMobEffectPacket

Remove a potion effect.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | entityId | Entity ID. |
| 4 | `byte` | effectId | Effect ID to remove. |

**Total payload: 5 bytes.**

@subsubsection pkt_43 Packet 43 - SetExperiencePacket

Update the XP bar.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `float` | progress | XP bar fill amount (0.0-1.0). |
| 4 | `short` | level | Current level. |
| 6 | `short` | totalXP | Total experience points. |

**Total payload: 8 bytes.**

@subsubsection pkt_71 Packet 71 - AddGlobalEntityPacket

Spawn a global entity (lightning bolt).

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | entityId | Entity ID. |
| 4 | `byte` | type | Entity type (1 = lightning bolt). |
| 5 | `int` | x | X coordinate * 32 (fixed-point). |
| 9 | `int` | y | Y coordinate * 32 (fixed-point). |
| 13 | `int` | z | Z coordinate * 32 (fixed-point). |

**Total payload: 17 bytes.**

@subsection packet_ref_player Player Packets

@subsubsection pkt_5 Packet 5 - SetEquippedItemPacket

Change the visible held item for an entity.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | entityId | Entity ID. |
| 4 | `short` | slot | Equipment slot (0=held). |
| 6 | `item` | item | Item data (see \ref item_data "Item Data"). |

**Total payload: 6 bytes + item data** (2 bytes if empty, 5 bytes if present).

@subsubsection pkt_200 Packet 200 - AwardStatPacket

Award a statistic or achievement.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `int` | statId | Statistic/achievement ID. |
| 4 | `int` | length | Length of parameter data blob in bytes. |
| 8 | `byte[length]` | data | Parameter data (typically a 4-byte `int` count). |

**Total payload: 8 bytes + length bytes.** For a simple stat increment, `length` = 4 and `data` contains an `int` count.

@subsubsection pkt_202 Packet 202 - PlayerAbilitiesPacket

Update player abilities.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `byte` | flags | Bitfield: `0x01`=invulnerable, `0x02`=flying, `0x04`=canFly, `0x08`=instabuild (creative). |
| 1 | `float` | flySpeed | Fly speed (default 0.05). |
| 5 | `float` | walkSpeed | Walk speed (default 0.1). |

**Total payload: 9 bytes.**

@subsection packet_ref_container Container Packets

@subsubsection pkt_100 Packet 100 - ContainerOpenPacket

Open a container window.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `byte` | containerId | Window ID. |
| 1 | `byte` | type | Container type (0=chest, 1=workbench, 2=furnace, 3=dispenser, 4=enchanting table). |
| 2 | `byte` | size | Number of slots. |
| 3 | `bool` | customName | Whether a custom title follows. |

If `type` == HORSE (type 12):

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 4 | `int` | entityId | Horse entity ID. |

If `customName` is true:

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| ... | `utf` | title | Custom container title. |

**Total payload: 4 bytes minimum**, variable with conditionals.

@subsubsection pkt_101 Packet 101 - ContainerClosePacket

Close a container window.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `byte` | containerId | Window ID. |

**Total payload: 1 byte.**

@subsubsection pkt_103 Packet 103 - ContainerSetSlotPacket

Set a single slot in a container.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `byte` | containerId | Window ID. |
| 1 | `short` | slot | Slot index. |
| 3 | `item` | item | Item data (see \ref item_data "Item Data"). |

**Total payload: 3 bytes + item data** (2 bytes if empty, 5 bytes if present).

@subsubsection pkt_104 Packet 104 - ContainerSetContentPacket

Set all slots in a container.

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0 | `byte` | containerId | Window ID. |
| 1 | `short` | count | Number of item slots. |
| 3 | `item[count]` | items | Array of item data entries. |

**Total payload: 3 bytes + count * item data.**

@subsection packet_ref_all_ids All Packet IDs

Every registered packet. **S-C** = server to client, **C-S** = client to server.

Just know that some of these packets have no function (such as scoreboards), most should work.

| ID | Name | S-C | C-S | Notes |
|----|------|:---:|:---:|-------|
| 0 | KeepAlivePacket | ✓ | ✓ | Connection keepalive. |
| 1 | LoginPacket | ✓ | ✓ | Login handshake. |
| 2 | PreLoginPacket | ✓ | ✓ | Pre-login handshake. |
| 3 | ChatPacket | ✓ | ✓ | Chat messages. |
| 4 | SetTimePacket | ✓ | | World time. |
| 5 | SetEquippedItemPacket | ✓ | | Held item display. |
| 6 | SetSpawnPositionPacket | ✓ | | World spawn point. |
| 7 | InteractPacket | | ✓ | Player interact with entity. |
| 8 | SetHealthPacket | ✓ | | Health/food/saturation. |
| 9 | RespawnPacket | ✓ | ✓ | Respawn/dimension change. |
| 10 | MovePlayerPacket | ✓ | ✓ | Player position (base). |
| 11 | MovePlayerPacket.Pos | ✓ | ✓ | Player position only. |
| 12 | MovePlayerPacket.Rot | ✓ | ✓ | Player rotation only. |
| 13 | MovePlayerPacket.PosRot | ✓ | ✓ | Player position + rotation. |
| 14 | PlayerActionPacket | | ✓ | Block breaking, item dropping. |
| 15 | UseItemPacket | | ✓ | Place block / use item. |
| 16 | SetCarriedItemPacket | ✓ | ✓ | Hotbar slot selection. |
| 17 | EntityActionAtPositionPacket | ✓ | | Sleep in bed. |
| 18 | AnimatePacket | ✓ | ✓ | Entity animations. |
| 19 | PlayerCommandPacket | | ✓ | Sneak, sprint, etc. |
| 20 | AddPlayerPacket | ✓ | | Spawn named player. |
| 22 | TakeItemEntityPacket | ✓ | | Item pickup animation. |
| 23 | AddEntityPacket | ✓ | | Spawn non-mob entity. |
| 24 | AddMobPacket | ✓ | | Spawn mob. |
| 25 | AddPaintingPacket | ✓ | | Spawn painting. |
| 26 | AddExperienceOrbPacket | ✓ | | Spawn XP orb. |
| 27 | PlayerInputPacket | | ✓ | Vehicle steering input. |
| 28 | SetEntityMotionPacket | ✓ | | Entity velocity. |
| 29 | RemoveEntitiesPacket | ✓ | | Despawn entities. |
| 30 | MoveEntityPacket | ✓ | | Entity movement (base). |
| 31 | MoveEntityPacket.Pos | ✓ | | Entity position delta. |
| 32 | MoveEntityPacket.Rot | ✓ | | Entity rotation. |
| 33 | MoveEntityPacket.PosRot | ✓ | | Entity pos + rot delta. |
| 34 | TeleportEntityPacket | ✓ | | Entity absolute position. |
| 35 | RotateHeadPacket | ✓ | | Entity head rotation. |
| 38 | EntityEventPacket | ✓ | | Entity events (hurt, death). |
| 39 | SetEntityLinkPacket | ✓ | | Leash / riding. |
| 40 | SetEntityDataPacket | ✓ | | Entity metadata update. |
| 41 | UpdateMobEffectPacket | ✓ | | Apply potion effect. |
| 42 | RemoveMobEffectPacket | ✓ | | Remove potion effect. |
| 43 | SetExperiencePacket | ✓ | | XP bar update. |
| 44 | UpdateAttributesPacket | ✓ | | Entity attributes. |
| 50 | ChunkVisibilityPacket | ✓ | | Chunk visibility. |
| 51 | BlockRegionUpdatePacket | ✓ | | Chunk data. |
| 52 | ChunkTilesUpdatePacket | ✓ | | Multi-block change. |
| 53 | TileUpdatePacket | ✓ | | Single block change. |
| 54 | TileEventPacket | ✓ | | Block action. |
| 55 | TileDestructionPacket | ✓ | | Block breaking animation. |
| 60 | ExplodePacket | ✓ | | Explosion. |
| 61 | LevelEventPacket | ✓ | | World event (sounds, particles). |
| 62 | LevelSoundPacket | ✓ | | Sound effect. |
| 63 | LevelParticlesPacket | ✓ | | Particle effect. |
| 70 | GameEventPacket | ✓ | | Game state change. |
| 71 | AddGlobalEntityPacket | ✓ | | Lightning bolt. |
| 100 | ContainerOpenPacket | ✓ | | Open container. |
| 101 | ContainerClosePacket | ✓ | ✓ | Close container. |
| 102 | ContainerClickPacket | | ✓ | Click container slot. |
| 103 | ContainerSetSlotPacket | ✓ | ✓ | Set container slot. |
| 104 | ContainerSetContentPacket | ✓ | | Set all container slots. |
| 105 | ContainerSetDataPacket | ✓ | | Container progress bar data. |
| 106 | ContainerAckPacket | ✓ | ✓ | Transaction acknowledgement. |
| 107 | SetCreativeModeSlotPacket | ✓ | ✓ | Creative inventory action. |
| 108 | ContainerButtonClickPacket | | ✓ | Enchanting / other button. |
| 130 | SignUpdatePacket | ✓ | ✓ | Sign text update. |
| 131 | ComplexItemDataPacket | ✓ | | Map data. |
| 132 | TileEntityDataPacket | ✓ | | Tile entity NBT data. |
| 133 | TileEditorOpenPacket | ✓ | | Open tile entity editor. |
| 200 | AwardStatPacket | ✓ | | Award statistic. |
| 201 | PlayerInfoPacket | ✓ | ✓ | Player list info. |
| 202 | PlayerAbilitiesPacket | ✓ | ✓ | Player abilities. |
| 206 | SetObjectivePacket | ✓ | | Scoreboard objective. |
| 207 | SetScorePacket | ✓ | | Scoreboard score. |
| 208 | SetDisplayObjectivePacket | ✓ | | Scoreboard display slot. |
| 209 | SetPlayerTeamPacket | ✓ | | Scoreboard team. |
| 255 | DisconnectPacket | ✓ | ✓ | Disconnect. |

---

@section where_to_look Where to Look

- **\ref Minecraft.Server.FourKit.Experimental.PlayerConnection "PlayerConnection"** - Get it via `player.getConnection()`. This is what you call `send()` on.
- **\ref Minecraft.Server.FourKit.Sound "Sound"** - All sound type IDs for LevelSoundPacket (62). Cast to `int` for the wire value.
- **\ref Minecraft.Server.FourKit.GameMode "GameMode"** - Game mode constants for GameEventPacket (70) and PlayerAbilitiesPacket (202).
- **\ref Minecraft.Server.FourKit.Location "Location"** - Get world coordinates from `player.getLocation()` for position encoding.
- **Packet source files** - The definitive reference is the `write()` method of each packet class in thre code (for example `LevelSoundPacket.cpp`, `TeleportEntityPacket.cpp`).

---

@section extra-info Extra info

- **Packet IDs** are a single byte. Only the low byte matters on the wire.
- **Position values** in entity packets use *32 fixed-point. Sound event positions use *8.
- **Rotation angles** are a single byte: `(byte)(angle * 256.0 / 360.0)`.
- **Velocity** is `(short)(velocity * 8000.0)`, clamped to +/-3.9 blocks/tick before encoding.
- **Strings** use modified UTF-8 with a 2-byte length prefix.
- **Item data** writes `-1` as a short for empty slots (2 bytes), or short ID + byte count + short damage for occupied slots (5 bytes).
- The 4-byte size header is written by the server automatically - don't include it in your byte array.
- FourKit's \ref Minecraft.Server.FourKit.Sound "Sound" enum values map directly to the sound IDs used in LevelSoundPacket. VERY USEFUL!!!!
- If you send malformed data, the client will likely disconnect or crash, or do some funny weird stuff. Test carefully.
- Some packets have conditional fields (ExplodePacket, ContainerOpenPacket, SetEntityMotionPacket). Read the wire format tables above carefully.
- **Entity metadata** - see \ref metadata_encoding "Metadata Encoding" for how to construct SynchedEntityData blobs by hand.
