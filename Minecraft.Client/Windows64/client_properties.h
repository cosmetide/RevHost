#pragma once

#ifdef _WINDOWS64

#include <cstdio>
#include <cstring>
#include <Windows.h>

namespace ClientProps
{
	inline void GetExeDir(char* out, size_t outSize)
	{
		out[0] = '\0';
		char exePath[MAX_PATH] = {};
		DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
		if (len == 0 || len >= MAX_PATH) return;
		char* lastSlash = strrchr(exePath, '\\');
		if (lastSlash) { *(lastSlash + 1) = '\0'; }
		strcpy_s(out, outSize, exePath);
	}

	inline void BuildFilePath(char* out, size_t outSize)
	{
		char dir[MAX_PATH] = {};
		GetExeDir(dir, sizeof(dir));
		_snprintf_s(out, outSize, _TRUNCATE, "%sclient.properties", dir);
	}

	inline bool GetValue(const char* key, char* value, size_t valueSize, const char* defaultVal)
	{
		value[0] = '\0';
		char path[MAX_PATH] = {};
		BuildFilePath(path, sizeof(path));

		FILE* f = nullptr;
		if (fopen_s(&f, path, "r") != 0 || f == nullptr) {
			if (defaultVal) strcpy_s(value, valueSize, defaultVal);
			return false;
		}

		char line[512];
		size_t keyLen = strlen(key);
		bool found = false;

		while (fgets(line, sizeof(line), f))
		{
			int len = (int)strlen(line);
			while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
				line[--len] = '\0';

			if (len == 0 || line[0] == '#') continue;

			if (strncmp(line, key, keyLen) == 0 && line[keyLen] == '=')
			{
				const char* val = line + keyLen + 1;
				strncpy_s(value, valueSize, val, _TRUNCATE);
				found = true;
				break;
			}
		}
		fclose(f);

		if (!found && defaultVal)
			strcpy_s(value, valueSize, defaultVal);

		return found;
	}

	inline void SetValue(const char* key, const char* value)
	{
		char path[MAX_PATH] = {};
		BuildFilePath(path, sizeof(path));

		// Read all existing lines
		char lines[64][512];
		int lineCount = 0;
		size_t keyLen = strlen(key);
		bool replaced = false;

		FILE* f = nullptr;
		if (fopen_s(&f, path, "r") == 0 && f != nullptr)
		{
			while (lineCount < 64 && fgets(lines[lineCount], sizeof(lines[0]), f))
			{
				int len = (int)strlen(lines[lineCount]);
				while (len > 0 && (lines[lineCount][len - 1] == '\n' || lines[lineCount][len - 1] == '\r'))
					lines[lineCount][--len] = '\0';

				if (strncmp(lines[lineCount], key, keyLen) == 0 && lines[lineCount][keyLen] == '=')
				{
					_snprintf_s(lines[lineCount], sizeof(lines[0]), _TRUNCATE, "%s=%s", key, value);
					replaced = true;
				}
				lineCount++;
			}
			fclose(f);
		}

		// Write back
		if (fopen_s(&f, path, "w") != 0 || f == nullptr) return;

		for (int i = 0; i < lineCount; i++)
		{
			fprintf_s(f, "%s\n", lines[i]);
		}
		if (!replaced)
		{
			fprintf_s(f, "%s=%s\n", key, value);
		}
		fclose(f);
	}

	inline void LoadAll(char* username, size_t usernameSize,
						char* uid, size_t uidSize)
	{
		GetValue("username", username, usernameSize, "Server");
		GetValue("uid", uid, uidSize, "");

		// Create file with defaults if it doesn't exist
		char path[MAX_PATH] = {};
		BuildFilePath(path, sizeof(path));
		DWORD attrs = GetFileAttributesA(path);
		if (attrs == INVALID_FILE_ATTRIBUTES)
		{
			FILE* f = nullptr;
			if (fopen_s(&f, path, "w") == 0 && f != nullptr)
			{
				fprintf_s(f, "# NeoHost Client Configuration\n");
				fprintf_s(f, "username=%s\n", username);
				fprintf_s(f, "uid=\n");
				fclose(f);
			}
		}
	}
}

// Global client properties
extern char g_CP_Username[17];
extern char g_CP_Uid[32];

#endif
