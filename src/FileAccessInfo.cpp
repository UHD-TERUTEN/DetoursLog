#include "FileAccessInfo.h"

#include <fstream>

extern std::ofstream logger;
extern std::ofstream modules;

inline static std::string BoolToText(BOOL value)
{
    return (value == TRUE) ? "TRUE" : "FALSE";
}

namespace LogData
{
    FileAccessInfo GetFileAccessInfo(const std::string& functionName, BOOL returnValue)
    {
        return FileAccessInfo{ functionName, returnValue, GetLastError() };
    }

    void Log(const FileAccessInfo& fileAccessInfo)
	{
        logger  << "Function name:\t"   << fileAccessInfo.functionName              << std::endl
                << "Return value:\t"    << BoolToText(fileAccessInfo.returnValue)   << std::endl
                << "Error code:\t"      << fileAccessInfo.errorCode                 << std::endl;
	}
}