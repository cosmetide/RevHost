#pragma once

namespace ServerRuntime
{
	namespace Security
	{
		struct SecuritySettings
		{
			bool hidePlayerListPreLogin = true;
			int rateLimitConnectionsPerWindow = 5;
			int rateLimitWindowSeconds = 30;
			int maxPendingConnections = 10;
			bool requireChallengeToken = false;
			bool enableStreamCipher = true;
			bool requireSecureClient = true;
			bool proxyProtocol = false;
		};

		void InitializeSettings(const SecuritySettings &settings);
		const SecuritySettings &GetSettings();
	}
}
