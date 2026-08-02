#pragma once

#include <string>
#include <vector>
#include <stdarg.h>

#include "../Minecraft.World/DisconnectPacket.h"

namespace ServerRuntime
{
    namespace ServerLogManager
    {
        /**
         * Identifies why the dedicated server rejected a TCP connection before login completed
         * ログイン完了前にTCP接続を拒否した理由
         */
        enum ETcpRejectReason
        {
            eTcpRejectReason_BannedIp = 0,
            eTcpRejectReason_GameNotReady,
            eTcpRejectReason_ServerFull,
            eTcpRejectReason_RateLimited,
            eTcpRejectReason_TooManyPending,
            eTcpRejectReason_InvalidProxyHeader
        };

        /**
         * Identifies why the dedicated server rejected a player during login validation
         * ログイン検証中にプレイヤーを拒否した理由
         */
        enum ELoginRejectReason
        {
            eLoginRejectReason_BannedXuid = 0,
            eLoginRejectReason_NotWhitelisted,
            eLoginRejectReason_DuplicateXuid,
            eLoginRejectReason_DuplicateName
        };

        /**
         * Returns `true` when client-side debug logs should be redirected into the dedicated server logger
         * dedicated server時にclient側デバッグログを転送すかどうか
         */
        bool ShouldForwardClientDebugLogs();

        /**
         * Formats and forwards `CMinecraftApp::DebugPrintf` output through the dedicated server logger
         * CMinecraftApp::DebugPrintf の出力を専用サーバーロガーへ転送
         */
        void ForwardClientAppDebugLogV(const char *format, va_list args);

        /**
         * Formats and forwards `CMinecraftApp::DebugPrintf(int user, ...)` output through the dedicated server logger
         * CMinecraftApp::DebugPrintf(int user, ...) の出力を専用サーバーロガーへ転送
         */
        void ForwardClientUserDebugLogV(int user, const char *format, va_list args);

        /**
         * Formats and forwards legacy `DebugSpew` output through the dedicated server logger
         * 従来の DebugSpew 出力を専用サーバーロガーへ転送
         */
        void ForwardClientDebugSpewLogV(const char *format, va_list args);

        /**
         * Clears cached connection metadata before the dedicated server starts accepting players
         * 接続ログ管理用のキャッシュを初期化
         */
        void Initialize();

        /**
         * Releases cached connection metadata after the dedicated server stops
         * 接続ログ管理用のキャッシュを停止時に破棄
         */
        void Shutdown();

        /**
         * **Log Incoming TCP Connection**
         *
         * Emits a named log for a raw TCP accept before smallId assignment finishes
         * smallId割り当て前のTCP接続を記録
         */
        void OnIncomingTcpConnection(const char *ip);

        /**
         * Emits a named log for a TCP connection rejected before login starts
         * ログイン開始前に拒否したTCP接続を記録
         */
        void OnRejectedTcpConnection(const char *ip, ETcpRejectReason reason);

        /**
         * Stores the remote IP for the assigned smallId and logs the accepted transport connection
         * 割り当て済みsmallIdに対接続IPを保存して記録
         */
        void OnAcceptedTcpConnection(unsigned char smallId, const char *ip);

        /**
         * Associates a player name and identity with the connection and emits the accepted login log
         */
        void OnAcceptedPlayerLogin(unsigned char smallId, const std::wstring &playerName,
            PlayerUID offlineXuid = INVALID_XUID, PlayerUID onlineXuid = INVALID_XUID, bool isGuest = false);

        // Security milestone recording -- accumulates per-connection state for the
        // consolidated "player secured" summary line
        void OnCipherHandshakeCompleted(unsigned char smallId);
        void OnCipherCompletedNoTokenRequired(unsigned char smallId);
        void OnIdentityTokenIssued(unsigned char smallId);
        void OnIdentityTokenVerified(unsigned char smallId);
        void OnIdentityTokenTimeout(unsigned char smallId, const std::wstring &playerName);

        // Security warnings -- emit immediately to CLI
        void OnIdentityTokenMismatch(unsigned char smallId, const std::wstring &playerName);
        void OnIdentityTokenTimeout(unsigned char smallId, const std::wstring &playerName);
        void OnUnsecuredClientKicked(unsigned char smallId);
        void OnXuidSpoofDetected(unsigned char smallId, const std::wstring &claimedName,
            const char *newIp, const char *existingIp);
        void OnUnauthorizedCommand(unsigned char smallId, const std::wstring &playerName, const char *action);

        /**
         * Emits a named login rejection log and clears cached metadata for that smallId
         * ログイン拒否を記録し対象smallIdのキャッシュを破棄
         */
        void OnRejectedPlayerLogin(unsigned char smallId, const std::wstring &playerName, ELoginRejectReason reason);

        /**
         * Emits a named disconnect log using cached connection metadata and then clears that entry
         * 接続キャッシュを使って切断ログを出しその後で破棄
         */
        void OnPlayerDisconnected(
            unsigned char smallId,
            const std::wstring &playerName,
            DisconnectPacket::eDisconnectReason reason,
            bool initiatedByServer);

        /**
         * Reads the cached remote IP for a live smallId without consuming the entry
         * Eventually, this should be implemented in a separate class or on the `Minecraft.Client` side instead.
         * 
         * 指定smallIdの接続IPをキャッシュから参照する
         */
        bool TryGetConnectionRemoteIp(unsigned char smallId, std::string *outIp);

        /**
         * Removes any remembered IP or player name for the specified smallId
         * 指定smallIdに紐づく接続キャッシュを消去
         */
        void ClearConnection(unsigned char smallId);

        /**
         * Cache a player name -> XUID mapping from a login attempt (accepted or rejected).
         * Used by `whitelist add <name>` to resolve names to XUIDs.
         */
        void CachePlayerXuid(const std::wstring &playerName, PlayerUID xuid);

        /**
         * Get all cached XUIDs for a player name (case-insensitive).
         * Returns the number of distinct XUIDs seen. If > 1, the name is ambiguous
         * and the operator should use an explicit XUID.
         *
         * Note: cached names are attacker-controlled (from LoginPacket). This cache
         * is an operator convenience tool, not a security mechanism.
         */
        int GetCachedXuids(const std::string &playerName, std::vector<PlayerUID> *outXuids);
    }
}
