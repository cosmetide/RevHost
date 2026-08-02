#include "stdafx.h"
#include "PendingConnection.h"
#include "PlayerConnection.h"
#include "ServerConnection.h"
#include "ServerPlayer.h"
#include "ServerPlayerGameMode.h"
#include "ServerLevel.h"
#include "PlayerList.h"
#include "MinecraftServer.h"
#include "../Minecraft.World/net.minecraft.network.h"
#include "../Minecraft.World/Pos.h"
#include "../Minecraft.World/net.minecraft.world.level.dimension.h"
#include "../Minecraft.World/net.minecraft.world.level.storage.h"
#include "../Minecraft.World/net.minecraft.world.item.h"
#include "../Minecraft.World/SharedConstants.h"
#include "Settings.h"
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
#include "../Minecraft.Server/ServerLogManager.h"
#include "../Minecraft.Server/Access/Access.h"
#include "../Minecraft.Server/Security/SecurityConfig.h"
#include "../Minecraft.World/Socket.h"
#include <FourKitBridge.h>
#include <Windows64/Network/WinsockNetLayer.h>
#endif
// #ifdef __PS3__
// #include "PS3/Network/NetworkPlayerSony.h"
// #endif

Random *PendingConnection::random = new Random();

#ifdef _WINDOWS64
bool g_bRejectDuplicateNames = true;
#endif

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
namespace
{
	static unsigned char GetPendingConnectionSmallId(Connection *connection)
	{
		if (connection != nullptr)
		{
			Socket *socket = connection->getSocket();
			if (socket != nullptr)
			{
				return socket->getSmallId();
			}
		}
		return 0;
	}
}
#endif

PendingConnection::PendingConnection(MinecraftServer *server, Socket *socket, const wstring& id)
{
	// 4J - added initialisers
	done = false;
	_tick = 0;
	name = L"";
	acceptedLogin = nullptr;
	loginKey = L"";

	this->server = server;
	connection = new Connection(socket, id, this);
	connection->fakeLag = FAKE_LAG;
}

PendingConnection::~PendingConnection()
{
	delete connection;
}

void PendingConnection::tick()
{
	if (acceptedLogin != nullptr)
	{
		this->handleAcceptedLogin(acceptedLogin);
		acceptedLogin = nullptr;
	}
	if (_tick++ == MAX_TICKS_BEFORE_LOGIN)
	{
		disconnect(DisconnectPacket::eDisconnect_LoginTooLong);
	}
	else
	{
		connection->tick();
	}
}

void PendingConnection::disconnect(DisconnectPacket::eDisconnectReason reason)
{
	//   try {	// 4J - removed try/catch
	//        logger.info("Disconnecting " + getName() + ": " + reason);
	app.DebugPrintf("Pending connection disconnect: %d\n", reason );
	connection->send(std::make_shared<DisconnectPacket>(reason));
	connection->sendAndQuit();
	done = true;
	//    } catch (Exception e) {
	//        e.printStackTrace();
	//    }
}

void PendingConnection::handlePreLogin(shared_ptr<PreLoginPacket> packet)
{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	ConsoleLog(L"Server received PreLogin from client");
#endif
	if (packet->m_netcodeVersion != MINECRAFT_NET_VERSION)
	{
		app.DebugPrintf("Netcode version is %d not equal to %d\n", packet->m_netcodeVersion, MINECRAFT_NET_VERSION);
		if (packet->m_netcodeVersion > MINECRAFT_NET_VERSION)
		{
			disconnect(DisconnectPacket::eDisconnect_OutdatedServer);
		}
		else
		{
			disconnect(DisconnectPacket::eDisconnect_OutdatedClient);
		}
		return;
	}

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	{
		std::string connectionIp = "";
		int connectionPort = 0;

		if (connection && connection->getSocket()) {
			unsigned char smallId = connection->getSocket()->getSmallId();
			if (smallId != 0) {
				if (!ServerRuntime::ServerLogManager::TryGetConnectionRemoteIp(smallId, &connectionIp))
				{
					SOCKET sock = WinsockNetLayer::GetSocketForSmallId(smallId);
					if (sock != INVALID_SOCKET)
					{
						sockaddr_in addr;
						int addrLen = sizeof(addr);
						if (getpeername(sock, (sockaddr*)&addr, &addrLen) == 0)
						{
							char ipBuf[64] = {};
							if (inet_ntop(AF_INET, &addr.sin_addr, ipBuf, sizeof(ipBuf)))
							{
								connectionIp = ipBuf;
								connectionPort = (int)ntohs(addr.sin_port);
							}
						}
					}
				} else {
					SOCKET sock = WinsockNetLayer::GetSocketForSmallId(smallId);
					if (sock != INVALID_SOCKET)
					{
						sockaddr_in addr;
						int addrLen = sizeof(addr);
						if (getpeername(sock, (sockaddr*)&addr, &addrLen) == 0)
							connectionPort = (int)ntohs(addr.sin_port);
					}
				}

				if (!connectionIp.empty()) {
					if (FourKitBridge::FirePlayerPreLogin(packet->loginKey, connectionIp, connectionPort)) {
						disconnect(DisconnectPacket::eDisconnect_EndOfStream);
						return;
					}
				}
			}
		}
	}
#endif

	// === Ban / Whitelist enforcement ===
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	if (ServerRuntime::Access::IsInitialized())
	{
		// Check IP ban for external connections
		if (connection && connection->getSocket() && !connection->getSocket()->isLocal())
		{
			unsigned char smallId = connection->getSocket()->getSmallId();
			std::string remoteIp;
			if (ServerRuntime::ServerLogManager::TryGetConnectionRemoteIp(smallId, &remoteIp))
			{
				if (ServerRuntime::Access::IsIpBanned(remoteIp))
				{
					disconnect(DisconnectPacket::eDisconnect_Banned);
					return;
				}
			}
		}

		// Check whitelist: if enabled, only whitelisted XUIDs can join
		if (ServerRuntime::Access::IsWhitelistEnabled())
		{
			// Local host always passes whitelist
			if (!connection || !connection->getSocket() || !connection->getSocket()->isLocal())
			{
				// We don't have XUID at PreLogin stage yet; the whitelist check
				// will be enforced at handleLogin/handleAcceptedLogin instead.
			}
		}
	}
#endif

	//	printf("Server: handlePreLogin\n");
	name = packet->loginKey; // 4J Stu - Change from the login packet as we know better on client end during the pre-login packet
	sendPreLoginResponse();
}

void PendingConnection::sendPreLoginResponse()
{
	// 4J Stu - Calculate the players with UGC privileges set
	PlayerUID *ugcXuids = new PlayerUID[MINECRAFT_NET_MAX_PLAYERS];
	DWORD ugcXuidCount = 0;
	DWORD hostIndex = 0;
	BYTE ugcFriendsOnlyBits = 0;
	char szUniqueMapName[14];

	StorageManager.GetSaveUniqueFilename(szUniqueMapName);

	PlayerList *playerList = MinecraftServer::getInstance()->getPlayers();
	for(auto& player : playerList->players)
	{
		// If the offline Xuid is invalid but the online one is not then that's guest which we should ignore
		// If the online Xuid is invalid but the offline one is not then we are definitely an offline game so dont care about UGC

		// PADDY - this is failing when a local player with chat restrictions joins an online game

		if( player != nullptr && player->connection->m_offlineXUID != INVALID_XUID && player->connection->m_onlineXUID != INVALID_XUID )
		{
			if( player->connection->m_friendsOnlyUGC )
			{
				ugcFriendsOnlyBits |= (1<<ugcXuidCount);
			}
			// Need to use the online XUID otherwise friend checks will fail on the client
			ugcXuids[ugcXuidCount] = player->connection->m_onlineXUID;

			if( player->connection->getNetworkPlayer() != nullptr && player->connection->getNetworkPlayer()->IsHost() ) hostIndex = ugcXuidCount;

			++ugcXuidCount;
		}
	}

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	// Security: strip real XUIDs from pre-login response to prevent unauthenticated enumeration.
	// The client receives the correct player count but cannot identify who is connected.
	// Real XUID data is sent post-login via PlayerInfoPacket broadcasts.
	if (ServerRuntime::Security::GetSettings().hidePlayerListPreLogin)
	{
		for (DWORD i = 0; i < ugcXuidCount; ++i)
		{
			ugcXuids[i] = INVALID_XUID;
		}
		ugcFriendsOnlyBits = 0;
	}
#endif

#if 0
	if (false)//	server->onlineMode) // 4J - removed
	{
		loginKey = L"TOIMPLEMENT"; // 4J - todo Long.toHexString(random.nextLong());
		connection->send( shared_ptr<PreLoginPacket>( new PreLoginPacket(loginKey, ugcXuids, ugcXuidCount, ugcFriendsOnlyBits, server->m_ugcPlayersVersion, szUniqueMapName,app.GetGameHostOption(eGameHostOption_All),hostIndex) ) );
	}
	else
#endif
	{
		DWORD cappedCount = (ugcXuidCount > 255u) ? 255u : ugcXuidCount;
		BYTE cappedHostIndex = (hostIndex >= 255u) ? 254 : static_cast<BYTE>(hostIndex);
		connection->send(std::make_shared<PreLoginPacket>(L"-", ugcXuids, cappedCount, ugcFriendsOnlyBits, server->m_ugcPlayersVersion, szUniqueMapName, app.GetGameHostOption(eGameHostOption_All), cappedHostIndex, server->m_texturePackId));
	}
}

void PendingConnection::handleLogin(shared_ptr<LoginPacket> packet)
{
	//	printf("Server: handleLogin\n");
	//name = packet->userName;
	if (packet->clientVersion != SharedConstants::NETWORK_PROTOCOL_VERSION)
	{
		app.DebugPrintf("Client version is %d not equal to %d\n", packet->clientVersion, SharedConstants::NETWORK_PROTOCOL_VERSION);
		if (packet->clientVersion > SharedConstants::NETWORK_PROTOCOL_VERSION)
		{
			disconnect(DisconnectPacket::eDisconnect_OutdatedServer);
		}
		else
		{
			disconnect(DisconnectPacket::eDisconnect_OutdatedClient);
		}
		return;
	}

	//if (true)// 4J removed !server->onlineMode)
	bool sentDisconnect = false;

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	{
		std::string connectionIp = "";
		int connectionPort = 0;

		if (!connection || !connection->getSocket()) {
			disconnect(DisconnectPacket::eDisconnect_EndOfStream);
			return;
		}

		if (!connection->getSocket()->isLocal())
		{
			unsigned char smallId = connection->getSocket()->getSmallId();
			if (smallId == 0) {
				disconnect(DisconnectPacket::eDisconnect_EndOfStream);
				return;
			}

			if (!ServerRuntime::ServerLogManager::TryGetConnectionRemoteIp(smallId, &connectionIp))
			{
				SOCKET sock = WinsockNetLayer::GetSocketForSmallId(smallId);
				if (sock != INVALID_SOCKET)
				{
					sockaddr_in addr;
					int addrLen = sizeof(addr);
					if (getpeername(sock, (sockaddr*)&addr, &addrLen) == 0)
					{
						char ipBuf[64] = {};
						if (inet_ntop(AF_INET, &addr.sin_addr, ipBuf, sizeof(ipBuf)))
						{
							connectionIp = ipBuf;
							connectionPort = (int)ntohs(addr.sin_port);
						}
					}
				}
				if (connectionIp.empty()) {
					disconnect(DisconnectPacket::eDisconnect_EndOfStream);
					return;
				}
			}
			else {
				SOCKET sock = WinsockNetLayer::GetSocketForSmallId(smallId);
				if (sock != INVALID_SOCKET)
				{
					sockaddr_in addr;
					int addrLen = sizeof(addr);
					if (getpeername(sock, (sockaddr*)&addr, &addrLen) == 0)
						connectionPort = (int)ntohs(addr.sin_port);
				}
			}

			if (FourKitBridge::FirePlayerLogin(packet->userName, connectionIp, connectionPort, 1, &packet->m_onlineXuid, &packet->m_offlineXuid)) {
				disconnect(DisconnectPacket::eDisconnect_EndOfStream);
				return;
			}
		}
	}
#endif

	// Use the same Xuid choice as handleAcceptedLogin (offline first, online fallback).
	// 
	PlayerUID loginXuid = packet->m_offlineXuid;
	if (loginXuid == INVALID_XUID) loginXuid = packet->m_onlineXuid;

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	// === XUID-based ban check (external connections only) ===
	if (ServerRuntime::Access::IsInitialized() && connection && connection->getSocket() && !connection->getSocket()->isLocal())
	{
		if (loginXuid != INVALID_XUID && ServerRuntime::Access::IsPlayerBanned(loginXuid))
		{
			disconnect(DisconnectPacket::eDisconnect_Banned);
			return;
		}

		// Whitelist check: only whitelisted XUIDs can join when whitelist is enabled
		if (ServerRuntime::Access::IsWhitelistEnabled() && loginXuid != INVALID_XUID)
		{
			if (!ServerRuntime::Access::IsPlayerWhitelisted(loginXuid))
			{
				disconnect(DisconnectPacket::eDisconnect_EndOfStream);
				return;
			}
		}
	}
#endif

	bool duplicateXuid = false;
	if (loginXuid != INVALID_XUID && server->getPlayers()->getPlayer(loginXuid) != nullptr)
	{
		duplicateXuid = true;
	}
	else if (packet->m_onlineXuid != INVALID_XUID &&
		packet->m_onlineXuid != loginXuid &&
		server->getPlayers()->getPlayer(packet->m_onlineXuid) != nullptr)
	{
		duplicateXuid = true;
	}

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	// Cross-reference: if someone claims the same XUID as an existing player from a different IP,
	// log and reject as a potential spoofing attempt.
	// Note: this runs on the main tick thread (via PendingConnection::tick -> Connection::tick ->
	// handleLogin), same thread that mutates the player list, so no lock is needed.
	if (!duplicateXuid && loginXuid != INVALID_XUID)
	{
		std::string newIp;
		unsigned char newSmallId = GetPendingConnectionSmallId(connection);
		bool hasNewIp = ServerRuntime::ServerLogManager::TryGetConnectionRemoteIp(newSmallId, &newIp);

		for (auto &existingPlayer : server->getPlayers()->players)
		{
			if (existingPlayer == nullptr) continue;
			PlayerUID existingXuid = existingPlayer->connection->m_offlineXUID;
			if (existingXuid == INVALID_XUID) existingXuid = existingPlayer->connection->m_onlineXUID;
			if (existingXuid == loginXuid)
			{
				if (hasNewIp)
				{
					std::string existingIp;
					INetworkPlayer *np = existingPlayer->connection->getNetworkPlayer();
					if (np != nullptr)
					{
						unsigned char existingSmallId = np->GetSmallId();
						if (ServerRuntime::ServerLogManager::TryGetConnectionRemoteIp(existingSmallId, &existingIp))
						{
							if (existingIp != newIp)
							{
								app.DebugPrintf("SECURITY: XUID spoofing suspected - XUID 0x%016llx claimed from IP %s while already connected from IP %s\n",
									(unsigned long long)loginXuid, newIp.c_str(), existingIp.c_str());
								ServerRuntime::ServerLogManager::OnXuidSpoofDetected(newSmallId, name, newIp.c_str(), existingIp.c_str());
								duplicateXuid = true;
							}
						}
					}
				}
				else
				{
					// Cannot verify IP -- treat same-XUID connection as suspicious
					app.DebugPrintf("SECURITY: XUID 0x%016llx claimed but could not verify source IP\n",
						(unsigned long long)loginXuid);
					duplicateXuid = true;
				}
				break;
			}
		}
	}
#endif

	bool bannedXuid = false;
	if (loginXuid != INVALID_XUID)
	{
		bannedXuid = server->getPlayers()->isXuidBanned(loginXuid);
	}
	if (!bannedXuid && packet->m_onlineXuid != INVALID_XUID && packet->m_onlineXuid != loginXuid)
	{
		bannedXuid = server->getPlayers()->isXuidBanned(packet->m_onlineXuid);
	}

	bool whitelistSatisfied = true;
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	if (ServerRuntime::Access::IsWhitelistEnabled())
	{
		whitelistSatisfied = false;
		if (loginXuid != INVALID_XUID)
		{
			whitelistSatisfied = ServerRuntime::Access::IsPlayerWhitelisted(loginXuid);
		}
		if (!whitelistSatisfied && packet->m_onlineXuid != INVALID_XUID && packet->m_onlineXuid != loginXuid)
		{
			whitelistSatisfied = ServerRuntime::Access::IsPlayerWhitelisted(packet->m_onlineXuid);
		}
	}
#endif

	if( sentDisconnect )
	{
		// Do nothing
	}
	else if (bannedXuid)
	{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		ServerRuntime::ServerLogManager::OnRejectedPlayerLogin(GetPendingConnectionSmallId(connection), name, ServerRuntime::ServerLogManager::eLoginRejectReason_BannedXuid);
#endif
		disconnect(DisconnectPacket::eDisconnect_Banned);
	}
	else if (!whitelistSatisfied)
	{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		// Cache name->XUID so `whitelist add <name>` can resolve the XUID
		ServerRuntime::ServerLogManager::CachePlayerXuid(name, loginXuid);
		ServerRuntime::ServerLogManager::OnRejectedPlayerLogin(GetPendingConnectionSmallId(connection), name, ServerRuntime::ServerLogManager::eLoginRejectReason_NotWhitelisted);
		app.DebugPrintf("WHITELIST: Rejected %ls (XUID: 0x%016llx) - use 'whitelist add %ls' to allow\n",
			name.c_str(), (unsigned long long)loginXuid, name.c_str());
#endif
		disconnect(DisconnectPacket::eDisconnect_Banned);
	}
	else if (duplicateXuid)
	{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		ServerRuntime::ServerLogManager::OnRejectedPlayerLogin(GetPendingConnectionSmallId(connection), name, ServerRuntime::ServerLogManager::eLoginRejectReason_DuplicateXuid);
#endif
		// Reject the incoming connection — a player with this UID is already
		// on the server.  Allowing duplicates causes invisible players and
		// other undefined behaviour.
		app.DebugPrintf("LOGIN: Rejecting duplicate xuid for name: %ls\n", name.c_str());
		disconnect(DisconnectPacket::eDisconnect_Banned);
	}
#ifdef _WINDOWS64
	else if (g_bRejectDuplicateNames)
	{
		bool nameTaken = false;
		vector<shared_ptr<ServerPlayer> >& pl = server->getPlayers()->players;
		for (const auto& i : pl)
		{
			if (i != nullptr && i->name == name)
			{
				nameTaken = true;
				break;
			}
		}
		if (nameTaken)
		{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			ServerRuntime::ServerLogManager::OnRejectedPlayerLogin(GetPendingConnectionSmallId(connection), name, ServerRuntime::ServerLogManager::eLoginRejectReason_DuplicateName);
#endif
			app.DebugPrintf("Rejecting duplicate name: %ls\n", name.c_str());
			disconnect(DisconnectPacket::eDisconnect_Banned);
		}
		else
		{
			handleAcceptedLogin(packet);
		}
	}
#endif
	else
	{
		handleAcceptedLogin(packet);
	}
	//else
	{
		//4J - removed
#if 0
		new Thread() {
			public void run() {
				try {
					String key = loginKey;
					URL url = new URL("http://www.minecraft.net/game/checkserver.jsp?user=" + URLEncoder.encode(packet.userName, "UTF-8") + "&serverId=" + URLEncoder.encode(key, "UTF-8"));
					BufferedReader br = new BufferedReader(new InputStreamReader(url.openStream()));
					String msg = br.readLine();
					br.close();
					if (msg.equals("YES")) {
						acceptedLogin = packet;
					} else {
						disconnect("Failed to verify username!");
					}
				} catch (Exception e) {
					disconnect("Failed to verify username! [internal error " + e + "]");
					e.printStackTrace();
				}
			}
		}.start();
#endif
	}

}

void PendingConnection::handleAcceptedLogin(shared_ptr<LoginPacket> packet)
{
	if(packet->m_ugcPlayersVersion != server->m_ugcPlayersVersion)
	{
		// Send the pre-login packet again with the new list of players
		sendPreLoginResponse();
		return;
	}

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	{
		std::string connectionIp = "";
		int connectionPort = 0;

		if (!connection || !connection->getSocket()) {
			disconnect(DisconnectPacket::eDisconnect_EndOfStream);
			return;
		}

		// Local (host) sockets don't have Winsock backing or remote IPs.
		// Skip the IP/port lookup and FirePlayerLogin hook for them.
		if (!connection->getSocket()->isLocal())
		{
			unsigned char smallId = connection->getSocket()->getSmallId();
			if (smallId == 0) {
				disconnect(DisconnectPacket::eDisconnect_EndOfStream);
				return;
			}

			if (!ServerRuntime::ServerLogManager::TryGetConnectionRemoteIp(smallId, &connectionIp))
			{
				SOCKET sock = WinsockNetLayer::GetSocketForSmallId(smallId);
				if (sock != INVALID_SOCKET)
				{
					sockaddr_in addr;
					int addrLen = sizeof(addr);
					if (getpeername(sock, (sockaddr*)&addr, &addrLen) == 0)
					{
						char ipBuf[64] = {};
						if (inet_ntop(AF_INET, &addr.sin_addr, ipBuf, sizeof(ipBuf)))
						{
							connectionIp = ipBuf;
							connectionPort = (int)ntohs(addr.sin_port);
						}
					}
				}
				if (connectionIp.empty()) {
					disconnect(DisconnectPacket::eDisconnect_EndOfStream);
					return;
				}
			}
			else {
				SOCKET sock = WinsockNetLayer::GetSocketForSmallId(smallId);
				if (sock != INVALID_SOCKET)
				{
					sockaddr_in addr;
					int addrLen = sizeof(addr);
					if (getpeername(sock, (sockaddr*)&addr, &addrLen) == 0)
						connectionPort = (int)ntohs(addr.sin_port);
				}
			}

			if (FourKitBridge::FirePlayerLogin(packet->userName, connectionIp, connectionPort, 2, &packet->m_onlineXuid, &packet->m_offlineXuid)) {
				disconnect(DisconnectPacket::eDisconnect_EndOfStream);
				return;
			}
		}
	}
#endif

	// Guests use the online xuid, everyone else uses the offline one
	PlayerUID playerXuid = packet->m_offlineXuid;
	if(playerXuid == INVALID_XUID) playerXuid = packet->m_onlineXuid;

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	// Cache name->XUID for console commands (whitelist add, revoketoken, etc.)
	ServerRuntime::ServerLogManager::CachePlayerXuid(name, playerXuid);
#endif

	shared_ptr<ServerPlayer> playerEntity = server->getPlayers()->getPlayerForLogin(this, name, playerXuid,packet->m_onlineXuid);
	if (playerEntity != nullptr)
	{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		ServerRuntime::ServerLogManager::OnAcceptedPlayerLogin(GetPendingConnectionSmallId(connection), name,
			packet->m_offlineXuid, packet->m_onlineXuid, packet->m_isGuest);
#endif
		server->getPlayers()->placeNewPlayer(connection, playerEntity, packet);
		connection = nullptr;	// We've moved responsibility for this over to the new PlayerConnection, nullptr so we don't delete our reference to it here in our dtor
	}
	done = true;

}

void PendingConnection::onDisconnect(DisconnectPacket::eDisconnectReason reason, void *reasonObjects)
{
	//    logger.info(getName() + " lost connection");
	done = true;
}

void PendingConnection::handleGetInfo(shared_ptr<GetInfoPacket> packet)
{
	//try {
	//String message = server->motd + "�" + server->players->getPlayerCount() + "�" + server->players->getMaxPlayers();
	//connection->send(new DisconnectPacket(message));
	connection->send(std::make_shared<DisconnectPacket>(DisconnectPacket::eDisconnect_ServerFull));
	connection->sendAndQuit();
	server->connection->removeSpamProtection(connection->getSocket());
	done = true;
	//} catch (Exception e) {
	//	e.printStackTrace();
	//}
}

void PendingConnection::handleKeepAlive(shared_ptr<KeepAlivePacket> packet)
{
	// Ignore
}

void PendingConnection::onUnhandledPacket(shared_ptr<Packet> packet)
{
	disconnect(DisconnectPacket::eDisconnect_UnexpectedPacket);
}

void PendingConnection::send(shared_ptr<Packet> packet)
{
	connection->send(packet);
}

wstring PendingConnection::getName()
{
	return L"Unimplemented";
	//        if (name != null) return name + " [" + connection.getRemoteAddress().toString() + "]";
	//        return connection.getRemoteAddress().toString();
}

bool PendingConnection::isServerPacketListener()
{
	return true;
}

bool PendingConnection::isDisconnected()
{
	return done;
}
