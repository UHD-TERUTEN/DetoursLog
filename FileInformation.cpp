#define WIN32_LEAN_AND_MEAN
#include "FileInformation.h"

#include <Windows.h>
#include <Shlwapi.h>    // SHFormatDateTimeA


#include <fstream>
#include <unordered_map>

#include "utilities.cpp"

extern std::ofstream logger;
extern std::ofstream report;

static std::unordered_map<HANDLE, std::wstring> filePathCache;

static
std::string GetFormatDateTime(FILETIME fileTime)
{
    // convert FILETIME to format date time
    DWORD pdwFlags = FDTF_DEFAULT;
    char formatDateTime[BUFSIZ] = { 0 };

    SHFormatDateTimeA(&fileTime, &pdwFlags, formatDateTime, sizeof(formatDateTime));

    return std::string(formatDateTime);
}


// https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getfileinformationbyhandleex
// https://stackoverflow.com/questions/4296096/how-to-get-the-name-of-a-file-from-a-file-handle-in-windows-using-c
std::optional<FileInformation> GetFileInformation(HANDLE hFile)
{
    BY_HANDLE_FILE_INFORMATION info;

    if (!GetFileInformationByHandle(hFile, &info))
    {
        return {};
    }

    std::wstring pathName;
    if (filePathCache.find(hFile) == filePathCache.end())
    {
        constexpr size_t PATHLEN = BUFSIZ * 4;
        TCHAR path[PATHLEN];

        GetFinalPathNameByHandle(hFile, path, PATHLEN, VOLUME_NAME_DOS);

        pathName = (filePathCache[hFile] = (path + 4));    // remove prepended '\\?\'
    }
    else
    {
        pathName = filePathCache[hFile];
    }

    auto a = ToString(pathName);
    auto b = (long long(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    auto c = GetFormatDateTime(info.ftCreationTime);
    auto d = GetFormatDateTime(info.ftLastWriteTime);
    auto e = static_cast<bool>(info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);
    return FileInformation{ a,b,c,d,e };

    //return FileStandardInformation
    //{
    //    ToString(pathName),
    //    (long long(info.nFileSizeHigh) << 32) | info.nFileSizeLow,
    //    GetFormatDateTime(info.ftCreationTime),
    //    GetFormatDateTime(info.ftLastWriteTime),
    //    static_cast<bool>(info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
    //};
}

void Log(const FileInformation& fileInfo)
{
    logger  << "���� �̸�:\t"   << fileInfo.fileName         << std::endl
            << "ũ��:\t\t"     << fileInfo.fileSize         << std::endl
            << "���� ��¥:\t"   << fileInfo.creationTime     << std::endl
            << "������ ��¥:\t" << fileInfo.lastWriteTime    << std::endl
            << "����:\t\t"     << fileInfo.isHidden         << std::endl;
}
