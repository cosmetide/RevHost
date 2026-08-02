#pragma once
using namespace std;

#include "Packet.h"

class LevelType;
class GameType;

class RespawnPacket : public Packet, public enable_shared_from_this<RespawnPacket>
{
public:
	char dimension;
	char difficulty;
	int64_t mapSeed;
    int mapHeight;
    GameType *playerGameType;
	bool m_newSeaLevel;	// 4J added
	LevelType *m_pLevelType;
	int m_newEntityId;
	int m_xzSize; // 4J Added
	int m_hellScale; // 4J Added
	bool m_isHardcore; // 4J Added - for hardcore mode

	RespawnPacket();
	RespawnPacket(char dimension, int64_t mapSeed, int mapHeight, GameType *playerGameType, char difficulty, LevelType *pLevelType, bool newSeaLevel, int newEntityId, int xzSize, int hellScale, bool isHardcore = false);

	virtual void handle(PacketListener *listener);
	virtual void read(DataInputStream *dis);
	virtual void write(DataOutputStream *dos);
	virtual int getEstimatedSize();

public:
	static shared_ptr<Packet> create() { return std::make_shared<RespawnPacket>(); }
	virtual int getId() { return 9; }
};
