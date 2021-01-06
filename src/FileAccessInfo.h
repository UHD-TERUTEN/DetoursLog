#pragma once
#include <string>

#include <Windows.h>

namespace LogData
{
	struct FileAccessInfo
	{
		std::string functionName;
		BOOL		returnValue;
		DWORD		errorCode;
	};

	FileAccessInfo GetFileAccessInfo(const std::string& functionName, BOOL returnValue);

	void Log(const FileAccessInfo& fileAccessInfo);
}