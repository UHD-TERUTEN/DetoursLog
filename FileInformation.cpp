#define WIN32_LEAN_AND_MEAN
#include "FileInformation.h"

#include <Windows.h>
#include <Shlwapi.h>    // SHFormatDateTimeA


#include <fstream>
#include <unordered_map>

#include "utilities.cpp"

//#define DEBUGFLAG

extern std::ofstream logger;
extern std::ofstream report;

static std::unordered_map<HANDLE, std::wstring> filePathCache;

static
std::string GetFormatDateTime(FILETIME fileTime)
{
    // convert FILETIME to format date time
    DWORD pdwFlags = FDTF_DEFAULT;
    char formatDateTime[BUFSIZ] = { 0 };

#ifdef DEBUGFLAG
    logger << 4 << std::endl;
#endif
    SHFormatDateTimeA(&fileTime, &pdwFlags, formatDateTime, sizeof(formatDateTime));

    return std::string(formatDateTime);
}


// https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getfileinformationbyhandleex
// https://stackoverflow.com/questions/4296096/how-to-get-the-name-of-a-file-from-a-file-handle-in-windows-using-c
std::optional<FileInformation> GetFileInformation(HANDLE hFile)
{
    BY_HANDLE_FILE_INFORMATION info;

#ifdef DEBUGFLAG
    logger << 1 << std::endl;
#endif
    if (!GetFileInformationByHandle(hFile, &info))
    {
        return {};
    }
#ifdef DEBUGFLAG
    logger << 2 << std::endl;
#endif

    std::wstring pathName;
    if (filePathCache.find(hFile) == filePathCache.end())
    {
        constexpr size_t PATHLEN = BUFSIZ * 4;
        TCHAR path[PATHLEN];
#ifdef DEBUGFLAG
        logger << 3.1 << std::endl;
#endif
        GetFinalPathNameByHandle(hFile, path, PATHLEN, VOLUME_NAME_DOS);

#ifdef DEBUGFLAG
        logger << 3.3 << std::endl;
#endif
        pathName = (filePathCache[hFile] = (path + 4));    // remove prepended '\\?\'
    }
    else
    {
#ifdef DEBUGFLAG
        logger << 3.9 << std::endl;
#endif
        pathName = filePathCache[hFile];
    }

#ifdef DEBUGFLAG
    logger << 'a' << std::endl;
#endif
    auto a = ToString(pathName);
#ifdef DEBUGFLAG
    logger << 'b' << std::endl;
#endif
    auto b = (long long(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
#ifdef DEBUGFLAG
    logger << 'c' << std::endl;
#endif
    auto c = GetFormatDateTime(info.ftCreationTime);
#ifdef DEBUGFLAG
    logger << 'd' << std::endl;
#endif
    auto d = GetFormatDateTime(info.ftLastWriteTime);
#ifdef DEBUGFLAG
    logger << 'e' << std::endl;
#endif
    auto e = static_cast<bool>(info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);
#ifdef DEBUGFLAG
    logger << 'ret' << std::endl;
#endif
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
    logger  << "파일 이름:\t"   << fileInfo.fileName         << std::endl
            << "크기:\t\t"     << fileInfo.fileSize         << std::endl
            << "만든 날짜:\t"   << fileInfo.creationTime     << std::endl
            << "수정한 날짜:\t" << fileInfo.lastWriteTime    << std::endl
            << "숨김:\t\t"     << fileInfo.isHidden         << std::endl;
}
