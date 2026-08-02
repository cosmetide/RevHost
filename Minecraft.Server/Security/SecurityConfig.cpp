#include "stdafx.h"
#include "SecurityConfig.h"

namespace ServerRuntime
{
	namespace Security
	{
		namespace
		{
			// Initialized once from main() before any worker threads start.
			// Default member initializers in SecuritySettings provide safe hardened
			// defaults if GetSettings() is called before InitializeSettings().
			// This global must NOT be written after threads are running.
			SecuritySettings g_settings;
		}

		void InitializeSettings(const SecuritySettings &settings)
		{
			g_settings = settings;
		}

		const SecuritySettings &GetSettings()
		{
			return g_settings;
		}
	}
}
