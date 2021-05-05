#include "FileAccessInfo.h"

namespace LogData
{
    FileAccessInfo MakeFileAccessInfo(const std::string& functionName, NTSTATUS returnValue)
    {
        return FileAccessInfo{ functionName, returnValue, GetLastError() };
    }
}