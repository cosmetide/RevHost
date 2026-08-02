#pragma once
#include <deque>
#include "../Minecraft.World/ArrayWithLength.h"

class ServerPlayer;
class PlayerChunkMap;
class MinecraftServer;
class PlayerIO;
class PendingConnection;
class Packet;
class ServerLevel;
class TileEntity;
class ProgressListener;
class GameType;
class LoginPacket;
class ServerScoreboard;

using namespace std;

class PlayerList
{
private:
	 static const int SEND_PLAYER_INFO_INTERVAL = 20 * 10;	// 4J - brought forward from 1.2.3
//    public static Logger logger = Logger.getLogger("Minecraft");
public:
	vector<shared_ptr<ServerPlayer> > players;
	CRITICAL_SECTION m_playersCS; // Protects players vector for concurrent access
	vector<shared_ptr<ServerPlayer> > getPlayersSnapshot();

private:
	MinecraftServer *server;
    unsigned int maxPlayers;

	// Pending disconnect queue: disconnect() enqueues here, tick() drains it.
	// This avoids holding done_cs across PlayerList operations (deadlock fix).
	struct PendingDisconnect
	{
		shared_ptr<ServerPlayer> player;
		int reason;
		wstring kickMessage;
		bool wasKicked;
		bool fourKitHandledQuit;
	};
	deque<PendingDisconnect> m_pendingDisconnects;
	CRITICAL_SECTION m_disconnectCS;

	// 4J Added
	vector<PlayerUID> m_bannedXuids;
	CRITICAL_SECTION m_banCS; // 4J Added - protects m_bannedXuids for concurrent access
	deque<BYTE> m_smallIdsToKick;
	CRITICAL_SECTION m_kickPlayersCS;
	deque<BYTE> m_smallIdsToClose;
	CRITICAL_SECTION m_closePlayersCS;
/* 4J - removed
	Set<String> bans = new HashSet<String>();
    Set<String> ipBans = new HashSet<String>();
    Set<String> ops = new HashSet<String>();
    Set<String> whitelist = new HashSet<String>();
    File banFile, ipBanFile, opFile, whiteListFile;
	*/
    PlayerIO *playerIo;
    bool doWhiteList;

	GameType *overrideGameMode;
	bool allowCheatsForAllPlayers;
	int viewDistance;

	int sendAllPlayerInfoIn;

	// 4J Added to maintain which players in which dimensions can receive all packet types
	vector<shared_ptr<ServerPlayer> > receiveAllPlayers[3];

	// Vanish system: track vanished player entity IDs
	std::unordered_set<int> m_vanishedEntityIds;
	CRITICAL_SECTION m_vanishCS;
private:
	shared_ptr<ServerPlayer> findAlivePlayerOnSystem(shared_ptr<ServerPlayer> currentPlayer);

public:
	void removePlayerFromReceiving(shared_ptr<ServerPlayer> player, bool usePlayerDimension = true, int dimension = 0);
	void addPlayerToReceiving(shared_ptr<ServerPlayer> player);
	bool canReceiveAllPackets(shared_ptr<ServerPlayer> player);

public:
	PlayerList(MinecraftServer *server);
	~PlayerList();
	bool placeNewPlayer(Connection *connection, shared_ptr<ServerPlayer> player, shared_ptr<LoginPacket> packet);

protected:
	void updateEntireScoreboard(ServerScoreboard *scoreboard, shared_ptr<ServerPlayer> player);

public:
    void setLevel(ServerLevelArray levels);
    void changeDimension(shared_ptr<ServerPlayer> player, ServerLevel *from);
    int getMaxRange();
	CompoundTag *load(shared_ptr<ServerPlayer> player);
protected:
	void save(shared_ptr<ServerPlayer> player);
public:
	void validatePlayerSpawnPosition(shared_ptr<ServerPlayer> player); // 4J Added
    void add(shared_ptr<ServerPlayer> player);
    void move(shared_ptr<ServerPlayer> player);
    void remove(shared_ptr<ServerPlayer> player);
	void queueDisconnect(shared_ptr<ServerPlayer> player, int reason, const wstring& kickMessage, bool wasKicked, bool fourKitHandledQuit);
	void drainPendingDisconnects();
    shared_ptr<ServerPlayer> getPlayerForLogin(PendingConnection *pendingConnection, const wstring& userName, PlayerUID xuid, PlayerUID OnlineXuid);
    shared_ptr<ServerPlayer> respawn(shared_ptr<ServerPlayer> serverPlayer, int targetDimension, bool keepAllPlayerData);
    void toggleDimension(shared_ptr<ServerPlayer> player, int targetDimension);
	void repositionAcrossDimension(shared_ptr<Entity> entity, int lastDimension, ServerLevel *oldLevel, ServerLevel *newLevel);
    void tick();
	bool isTrackingTile(int x, int y, int z, int dimension);		// 4J added
	void prioritiseTileChanges(int x, int y, int z, int dimension);	// 4J added
    void broadcastAll(shared_ptr<Packet> packet);
    void broadcastAll(shared_ptr<Packet> packet, int dimension);

    wstring getPlayerNames();

public:
	bool isWhiteListed(const wstring& name);
    bool isOp(const wstring& name);
	bool isOp(shared_ptr<ServerPlayer> player); // 4J Added
    shared_ptr<ServerPlayer> getPlayer(const wstring& name);
	shared_ptr<ServerPlayer> getPlayer(PlayerUID uid);
	shared_ptr<ServerPlayer> getNearestPlayer(Pos *position, int range);
	vector<ServerPlayer> *getPlayers(Pos *position, int rangeMin, int rangeMax, int count, int mode, int levelMin, int levelMax, unordered_map<wstring, int> *scoreRequirements, const wstring &playerName,	const wstring &teamName, Level *level);

private:
	bool meetsScoreRequirements(shared_ptr<Player> player, unordered_map<wstring, int> scoreRequirements);

public:
    void sendMessage(const wstring& name, const wstring& message);
    void broadcast(double x, double y, double z, double range, int dimension, shared_ptr<Packet> packet);
    void broadcast(shared_ptr<Player> except, double x, double y, double z, double range, int dimension, shared_ptr<Packet> packet);
	// 4J Added ProgressListener *progressListener param and bDeleteGuestMaps param
    void saveAll(ProgressListener *progressListener, bool bDeleteGuestMaps = false);
    void whiteList(const wstring& playerName);
    void blackList(const wstring& playerName);
//    Set<String> getWhiteList();		/ 4J removed
    void reloadWhitelist();
    void sendLevelInfo(shared_ptr<ServerPlayer> player, ServerLevel *level);
    void sendAllPlayerInfo(shared_ptr<ServerPlayer> player);
	int getPlayerCount();
	int getPlayerCount(ServerLevel *level); // 4J Added
	int getMaxPlayers();
	MinecraftServer *getServer();
	int getViewDistance();
	void setOverrideGameMode(GameType *gameMode);

private:
	void updatePlayerGameMode(shared_ptr<ServerPlayer> newPlayer, shared_ptr<ServerPlayer> oldPlayer, Level *level);

public:
	void setAllowCheatsForAllPlayers(bool allowCommands);

	// 4J Added
	void kickPlayerByShortId(BYTE networkSmallId);
	void closePlayerConnectionBySmallId(BYTE networkSmallId);
	void queueSmallIdForRecycle(BYTE smallId);
	bool isXuidBanned(PlayerUID xuid);
	void banXuid(PlayerUID xuid);  // 4J Added - for hardcore mode ban-on-death
	void banPlayerForHardcoreDeath(ServerPlayer *player);  // Persistent XUID + IP ban on hardcore death
	// AP added for Vita so the range can be increased once the level starts
	void setViewDistance(int newViewDistance);

	// Vanish system
	bool isVanished(shared_ptr<ServerPlayer> player);
	void setVanished(shared_ptr<ServerPlayer> player, bool vanished);
};
