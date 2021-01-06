#define WIN32_LEAN_AND_MEAN
#include "FileInfo.h"

#include <fstream>
#include <unordered_map>

#include <Windows.h>
#include <Shlwapi.h>    // SHFormatDateTimeA

#include "utilities.cpp"

extern std::ofstream logger;
extern std::ofstream modules;

static std::string GetFormatDateTime(FILETIME fileTime)
{
    // convert FILETIME to format date time
    DWORD pdwFlags = FDTF_DEFAULT;
    char formatDateTime[BUFSIZ]{};
    std::string dateTime;

    SHFormatDateTimeA(&fileTime, &pdwFlags, formatDateTime, sizeof(formatDateTime));
    dateTime = formatDateTime;
    dateTime.erase(std::remove(std::begin(dateTime), std::end(dateTime), '?'), std::end(dateTime));
    return dateTime;
}

namespace LogData
{
    FileInfo GetFileInfo(HANDLE hFile)
    {
        BY_HANDLE_FILE_INFORMATION handleFileInfo{};
        FileInfo info{};

        if (GetFileInformationByHandle(hFile, &handleFileInfo))
        {
            constexpr auto PATHLEN = BUFSIZ * 4;
            TCHAR path[PATHLEN]{};

            if (auto size = GetFinalPathNameByHandleW(hFile, path, PATHLEN, VOLUME_NAME_DOS);
                0 < size && size < PATHLEN)
            {
                info.fileName = ToString(path + 4);    // remove prepended '\\?\', network prepend('\\?\UNC\')?
                info.fileSize = ((long long)(handleFileInfo.nFileSizeHigh) << 32) | handleFileInfo.nFileSizeLow;
                info.creationTime = GetFormatDateTime(handleFileInfo.ftCreationTime);
                info.lastWriteTime = GetFormatDateTime(handleFileInfo.ftLastWriteTime);
                info.isHidden = static_cast<bool>(handleFileInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);
            }
        }
        return info;
    }

    void Log(const FileInfo& fileInfo)
    {
        logger  << "--------------------- [ File Information ]" << std::endl;
        logger  << "파일 이름:\t"   << fileInfo.fileName       << std::endl
                << "크기:\t\t"     << fileInfo.fileSize       << std::endl
                << "만든 날짜:\t"   << fileInfo.creationTime   << std::endl
                << "수정한 날짜:\t" << fileInfo.lastWriteTime  << std::endl
                << "숨김:\t\t"     << fileInfo.isHidden       << std::endl
                << std::endl;
    }
}