#pragma once
#include <cstdint>
#include <wchar.h>

#include <string>
#include <Windows.h>
using namespace std::literals;

struct FileInformation
{
    std::string fileName        = "(EMPTY)"s;
    long long   fileSize        = 0LL;
    std::string creationTime    = "(EMPTY)"s;
    std::string lastWriteTime   = "(EMPTY)"s;
    bool        isHidden        = false;
};

FileInformation GetFileInformation(HANDLE hFile);

void Log(const FileInformation& fileInfo);
