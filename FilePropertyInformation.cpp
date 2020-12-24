#define WIN32_LEAN_AND_MEAN
#include "FilePropertyInformation.h"

#include <Windows.h>
#include <WinBase.h>    // GetFileInformationByHandleEx
#include <winver.h>     // GetFileVersionInfo, VerQueryValue

#include <fstream>
#include <unordered_map>
#include <string>
#include <memory>
#include <cstring>
#include <sstream>
#include <iomanip>

#include "utilities.cpp"

//#define DEBUGFLAG

extern std::ofstream logger;
extern std::ofstream report;

static std::unordered_map<std::wstring, FilePropertyInformation> propertyCache;


inline static
std::string GetFileVersion(const VS_FIXEDFILEINFO* pFineInfo)
{
    std::stringstream ss;
    ss  << HIWORD(pFineInfo->dwFileVersionMS) << '.'
        << LOWORD(pFineInfo->dwFileVersionMS) << '.'
        << HIWORD(pFineInfo->dwFileVersionLS) << '.'
        << LOWORD(pFineInfo->dwFileVersionLS);
    return ss.str();
}

inline static
std::string GetProductVersion(const VS_FIXEDFILEINFO* pFineInfo)
{
    std::stringstream ss;
    ss  << HIWORD(pFineInfo->dwProductVersionMS) << '.'
        << LOWORD(pFineInfo->dwProductVersionMS) << '.'
        << HIWORD(pFineInfo->dwProductVersionLS) << '.'
        << LOWORD(pFineInfo->dwProductVersionLS);
    return ss.str();
}

inline static
std::string GetCompanyNameFormat(const WORD* langInfo)
{
    std::stringstream ss;
    ss.setf(std::ios::hex, std::ios::basefield);
    ss  << R"(\StringFileInfo\)"
        << std::setfill('0') << std::setw(4) << langInfo[0]
        << std::setfill('0') << std::setw(4) << langInfo[1]
        << R"(\CompanyName)";
    return ss.str();
}


// https://www.codeguru.com/cpp/w-p/win32/versioning/article.php/c4539/Versioning-in-Windows.htm
// https://smok95.tistory.com/105
FilePropertyInformation GetFilePropertyInformation(std::wstring path)
{
    FilePropertyInformation info;

    std::unique_ptr<wchar_t> infoBuffer;
    std::string companyName;

    if (propertyCache.find(path) != propertyCache.end())
        return propertyCache.at(path);

#ifdef DEBUGFLAG
    logger << "*1" << std::endl;
#endif

    auto infoSize = GetFileVersionInfoSize(path.c_str(), NULL);
    if (infoSize == 0)
        return info;
#ifdef DEBUGFLAG
    logger << "*2" << std::endl;
#endif

    if (infoBuffer = std::make_unique<wchar_t>(infoSize))
    {
#ifdef DEBUGFLAG
        logger << "*3" << std::endl;
#endif
        if (GetFileVersionInfo(path.c_str(), NULL, infoSize, infoBuffer.get()))
        {
#ifdef DEBUGFLAG
            logger << "*4" << std::endl;
#endif
            VS_FIXEDFILEINFO* pFineInfo;
            WORD* langInfo;
            LPCSTR value;
            UINT fineInfoSize, langInfoSize, valueSize;

            if (VerQueryValue(infoBuffer.get(), LR"(\)", (LPVOID*)&pFineInfo, &fineInfoSize))
            {
#ifdef DEBUGFLAG
                logger << "*5" << std::endl;
#endif
                info.fileVersion = GetFileVersion(pFineInfo);
            }

#ifdef DEBUGFLAG
            logger << "*6" << std::endl;
#endif
            // to get string information, we need to get language information.
            if (VerQueryValue(infoBuffer.get(), LR"(\VarFileInfo\Translation)", (LPVOID*)&langInfo, &langInfoSize))
            {
#ifdef DEBUGFLAG
                logger << "*7" << std::endl;
#endif
                auto format = GetCompanyNameFormat(langInfo);

                logger << format << std::endl;

                //Get the string from the resource data
                if (VerQueryValue(infoBuffer.get(), ToWstring(format).c_str(), (LPVOID*)&value, &valueSize))
                {
#ifdef DEBUGFLAG
                    logger << "*8" << std::endl;
#endif
                    logger << value << ' ' << valueSize << std::endl;
                    std::wstring companyName(value, value + valueSize * sizeof(TCHAR));
                    info.companyName = ToString(companyName);
                }
#ifdef DEBUGFLAG
                logger << "*9" << std::endl;
#endif
            }
#ifdef DEBUGFLAG
            logger << "*10" << std::endl;
#endif
        }
#ifdef DEBUGFLAG
        logger << "*11" << std::endl;
#endif
    }
#ifdef DEBUGFLAG
    logger << "*12" << std::endl;
#endif
    return (propertyCache[path] = info);
}

void LogFilePropertyInformation(const FilePropertyInformation& fileInfo)
{
    logger  << "ApplicationName:\t" << fileInfo.applicationName << std::endl
            << "FileVersion:\t"     << fileInfo.fileVersion     << std::endl
            << "CompanyName:\t"     << fileInfo.companyName     << std::endl;
}
