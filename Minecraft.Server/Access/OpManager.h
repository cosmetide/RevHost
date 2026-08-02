#pragma once

#include <string>
#include <vector>

namespace ServerRuntime
{
	namespace Access
	{
		struct OpMetadata
		{
			std::string created;
			std::string source;
		};

		struct OpPlayerEntry
		{
			std::string xuid;
			std::string name;
			OpMetadata metadata;
		};

		/**
		 * Persistent OP (operator) list manager.
		 *
		 * Stores XUID-based operator entries in `ops.json`.
		 * Used as the authoritative source of truth for who has OP privileges,
		 * preventing in-memory-only OP escalation via crafted packets.
		 */
		class OpManager
		{
		public:
			explicit OpManager(const std::string &baseDirectory = ".");

			bool EnsureOpFileExists() const;
			bool Reload();
			bool Save() const;

			bool LoadOps(std::vector<OpPlayerEntry> *outEntries) const;
			bool SaveOps(const std::vector<OpPlayerEntry> &entries) const;

			const std::vector<OpPlayerEntry> &GetOps() const;
			bool SnapshotOps(std::vector<OpPlayerEntry> *outEntries) const;

			bool IsPlayerOp(const std::string &xuid) const;
			bool AddOp(const OpPlayerEntry &entry);
			bool RemoveOpByXuid(const std::string &xuid);

			std::string GetOpFilePath() const;

			static OpMetadata BuildDefaultMetadata(const char *source = "Server");

		private:
			std::string BuildPath(const char *fileName) const;

		private:
			std::string m_baseDirectory;
			std::vector<OpPlayerEntry> m_ops;
		};
	}
}
