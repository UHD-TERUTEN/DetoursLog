#pragma once
#include <cstdint>
#include <wchar.h>

#include <string>
#include <optional>
#include <Windows.h>

struct FileInformation
{
    std::string fileName;
    long long   fileSize;
    std::string creationTime;
    std::string lastWriteTime;
    bool        isHidden;
};

std::optional<FileInformation> GetFileInformation(HANDLE hFile);

void Log(const FileInformation& fileInfo);
