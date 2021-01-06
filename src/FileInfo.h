#pragma once
#include <cstdint>
#include <string>
using namespace std::literals;

#include <Windows.h>
#include <wchar.h>

namespace LogData
{
    struct FileInfo
    {
        std::string fileName        = "(EMPTY)"s;
        long long   fileSize        = 0LL;
        std::string creationTime    = "(EMPTY)"s;
        std::string lastWriteTime   = "(EMPTY)"s;
        bool        isHidden        = false;
    };

    FileInfo GetFileInfo(HANDLE hFile);

    void Log(const FileInfo& fileInfo);
}