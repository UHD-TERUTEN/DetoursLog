#pragma once
#include <cstdint>
#include <wchar.h>

#include <string>
#include <optional>
#include <Windows.h>

struct FileStandardInformation
{
    std::string fileName;
    long long   fileSize;
    std::string creationTime;
    std::string lastWriteTime;
    bool        isHidden;
};

std::optional<FileStandardInformation> GetFileStandardInformation(HANDLE hFile);

void LogFileStandardInformation(const FileStandardInformation& fileInfo);
