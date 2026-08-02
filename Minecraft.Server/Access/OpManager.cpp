#include "stdafx.h"

#include "OpManager.h"

#include "../Common/AccessStorageUtils.h"
#include "../Common/FileUtils.h"
#include "../Common/StringUtils.h"
#include "../ServerLogger.h"
#include "../vendor/nlohmann/json.hpp"

#include <algorithm>

namespace ServerRuntime
{
	namespace Access
	{
		using OrderedJson = nlohmann::ordered_json;

		namespace
		{
			static const char *kOpFileName = "ops.json";
		}

		OpManager::OpManager(const std::string &baseDirectory)
			: m_baseDirectory(baseDirectory.empty() ? "." : baseDirectory)
		{
		}

		bool OpManager::EnsureOpFileExists() const
		{
			const std::string path = GetOpFilePath();
			if (!AccessStorageUtils::EnsureJsonListFileExists(path))
			{
				LogErrorf("access", "failed to create %s", path.c_str());
				return false;
			}
			return true;
		}

		bool OpManager::Reload()
		{
			std::vector<OpPlayerEntry> ops;
			if (!LoadOps(&ops))
			{
				return false;
			}

			m_ops.swap(ops);
			return true;
		}

		bool OpManager::Save() const
		{
			std::vector<OpPlayerEntry> ops;
			return SnapshotOps(&ops) && SaveOps(ops);
		}

		bool OpManager::LoadOps(std::vector<OpPlayerEntry> *outEntries) const
		{
			if (outEntries == nullptr)
			{
				return false;
			}
			outEntries->clear();

			std::string text;
			const std::string path = GetOpFilePath();
			if (!FileUtils::ReadTextFile(path, &text))
			{
				LogErrorf("access", "failed to read %s", path.c_str());
				return false;
			}

			if (text.empty())
			{
				text = "[]";
			}

			OrderedJson root;
			try
			{
				root = OrderedJson::parse(StringUtils::StripUtf8Bom(text));
			}
			catch (const nlohmann::json::exception &e)
			{
				LogErrorf("access", "failed to parse %s: %s", path.c_str(), e.what());
				return false;
			}

			if (!root.is_array())
			{
				LogErrorf("access", "failed to parse %s: root json value is not an array", path.c_str());
				return false;
			}

			for (const auto &object : root)
			{
				if (!object.is_object())
				{
					LogWarnf("access", "skipping op entry that is not an object in %s", path.c_str());
					continue;
				}

				std::string rawXuid;
				if (!AccessStorageUtils::TryGetStringField(object, "xuid", &rawXuid))
				{
					LogWarnf("access", "skipping op entry without xuid in %s", path.c_str());
					continue;
				}

				OpPlayerEntry entry;
				entry.xuid = AccessStorageUtils::NormalizeXuid(rawXuid);
				if (entry.xuid.empty())
				{
					LogWarnf("access", "skipping op entry with empty xuid in %s", path.c_str());
					continue;
				}

				AccessStorageUtils::TryGetStringField(object, "name", &entry.name);
				AccessStorageUtils::TryGetStringField(object, "created", &entry.metadata.created);
				AccessStorageUtils::TryGetStringField(object, "source", &entry.metadata.source);

				outEntries->push_back(entry);
			}

			return true;
		}

		bool OpManager::SaveOps(const std::vector<OpPlayerEntry> &entries) const
		{
			OrderedJson root = OrderedJson::array();
			for (const auto &entry : entries)
			{
				OrderedJson object = OrderedJson::object();
				object["xuid"] = AccessStorageUtils::NormalizeXuid(entry.xuid);
				object["name"] = entry.name;
				object["created"] = entry.metadata.created;
				object["source"] = entry.metadata.source;
				root.push_back(object);
			}

			const std::string path = GetOpFilePath();
			const std::string json = root.empty() ? std::string("[]\n") : (root.dump(2) + "\n");
			if (!FileUtils::WriteTextFileAtomic(path, json))
			{
				LogErrorf("access", "failed to write %s", path.c_str());
				return false;
			}
			return true;
		}

		const std::vector<OpPlayerEntry> &OpManager::GetOps() const
		{
			return m_ops;
		}

		bool OpManager::SnapshotOps(std::vector<OpPlayerEntry> *outEntries) const
		{
			if (outEntries == nullptr)
			{
				return false;
			}

			*outEntries = m_ops;
			return true;
		}

		bool OpManager::IsPlayerOp(const std::string &xuid) const
		{
			const auto normalized = AccessStorageUtils::NormalizeXuid(xuid);
			if (normalized.empty())
			{
				return false;
			}

			return std::any_of(
				m_ops.begin(),
				m_ops.end(),
				[&normalized](const OpPlayerEntry &entry)
				{
					return entry.xuid == normalized;
				});
		}

		bool OpManager::AddOp(const OpPlayerEntry &entry)
		{
			std::vector<OpPlayerEntry> updatedEntries;
			if (!SnapshotOps(&updatedEntries))
			{
				return false;
			}

			auto normalized = entry;
			normalized.xuid = AccessStorageUtils::NormalizeXuid(normalized.xuid);
			if (normalized.xuid.empty())
			{
				return false;
			}

			const auto existing = std::find_if(
				updatedEntries.begin(),
				updatedEntries.end(),
				[&normalized](const OpPlayerEntry &candidate)
				{
					return candidate.xuid == normalized.xuid;
				});

			if (existing != updatedEntries.end())
			{
				*existing = normalized;
				if (!SaveOps(updatedEntries))
				{
					return false;
				}

				m_ops.swap(updatedEntries);
				return true;
			}

			updatedEntries.push_back(normalized);
			if (!SaveOps(updatedEntries))
			{
				return false;
			}

			m_ops.swap(updatedEntries);
			return true;
		}

		bool OpManager::RemoveOpByXuid(const std::string &xuid)
		{
			const auto normalized = AccessStorageUtils::NormalizeXuid(xuid);
			if (normalized.empty())
			{
				return false;
			}

			std::vector<OpPlayerEntry> updatedEntries;
			if (!SnapshotOps(&updatedEntries))
			{
				return false;
			}

			const auto oldSize = updatedEntries.size();
			updatedEntries.erase(
				std::remove_if(
					updatedEntries.begin(),
					updatedEntries.end(),
					[&normalized](const OpPlayerEntry &entry) { return entry.xuid == normalized; }),
				updatedEntries.end());

			if (updatedEntries.size() == oldSize)
			{
				return false;
			}

			if (!SaveOps(updatedEntries))
			{
				return false;
			}

			m_ops.swap(updatedEntries);
			return true;
		}

		std::string OpManager::GetOpFilePath() const
		{
			return BuildPath(kOpFileName);
		}

		OpMetadata OpManager::BuildDefaultMetadata(const char *source)
		{
			OpMetadata metadata;
			metadata.created = StringUtils::GetCurrentUtcTimestampIso8601();
			metadata.source = (source != nullptr) ? source : "Server";
			return metadata;
		}

		std::string OpManager::BuildPath(const char *fileName) const
		{
			return AccessStorageUtils::BuildPathFromBaseDirectory(m_baseDirectory, fileName);
		}
	}
}
