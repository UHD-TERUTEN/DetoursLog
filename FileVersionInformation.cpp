#define WIN32_LEAN_AND_MEAN
#include "FileVersionInformation.h"

#include <WinBase.h>    // FindResource

#include <fstream>
#include <string>
#include <memory>
#include <sstream>
#include <iomanip>
#include <vector>
#include <functional>

#include "utilities.cpp"

extern std::ofstream logger;
extern std::ofstream report;

namespace FileVersionGetter
{
    static
        std::string GetLanguageCodePageString(const char* buffer)
    {
        struct LanguageCodePage
        {
            WORD language, codePage;
        } *translate{};
        auto bufsize = sizeof(LanguageCodePage);
        std::stringstream ss{};

        if (VerQueryValueA(buffer, R"(\VarFileInfo\Translation)", (LPVOID*)&translate, (PUINT)&bufsize))
        {
            ss.setf(std::ios::hex, std::ios::basefield);
            ss << std::setw(4) << std::setfill('0') << translate[0].language
                << std::setw(4) << std::setfill('0') << translate[0].codePage;
        }
        return ss.str();
    }

    static
        std::string GetStringName(const char* buffer, const char* key)
    {
        auto langCodePage = GetLanguageCodePageString(buffer);
        if (langCodePage.empty())
            langCodePage = "041204b0"s;

        std::stringstream ss;
        ss << R"(\StringFileInfo\)"
            << langCodePage
            << key;
        return ss.str();
    }

    inline static std::string GetCompanyName(const char* buffer)
    {
        return GetStringName(buffer, R"(\CompanyName)");
    }
    inline static std::string GetFileDescription(const char* buffer)
    {
        return GetStringName(buffer, R"(\FileDescription)");
    }
    inline static std::string GetFileVersion(const char* buffer)
    {
        return GetStringName(buffer, R"(\FileVersion)");
    }
    inline static std::string GetInternalName(const char* buffer)
    {
        return GetStringName(buffer, R"(\InternalName)");
    }
    inline static std::string GetLegalCopyright(const char* buffer)
    {
        return GetStringName(buffer, R"(\LegalCopyright)");
    }
    inline static std::string GetOriginalFilename(const char* buffer)
    {
        return GetStringName(buffer, R"(\OriginalFilename)");
    }
    inline static std::string GetProductName(const char* buffer)
    {
        return GetStringName(buffer, R"(\ProductName)");
    }
    inline static std::string GetProductVersion(const char* buffer)
    {
        return GetStringName(buffer, R"(\ProductVersion)");
    }

    static void SetStringFileInfo(FileVersionInformation& info, const char* buffer)
    {
        info.companyName = GetCompanyName(buffer);
        info.fileDescription = GetFileDescription(buffer);
        info.internalName = GetInternalName(buffer);
    }
}
using namespace FileVersionGetter;


// https://docs.microsoft.com/en-us/windows/win32/api/winver/nf-winver-verqueryvaluea
// https://docs.microsoft.com/en-us/windows/win32/menurc/version-information
FileVersionInformation GetFileVersionInformation(const std::string& fileName)
{
    FileVersionInformation info{};
    
    if (auto bufsize = GetFileVersionInfoSizeA(fileName.c_str(), NULL))
    {
        auto buffer = std::make_unique<char>(bufsize);

        if (GetFileVersionInfoA(fileName.c_str(), 0, bufsize, buffer.get()))
        {
            std::vector<std::pair<std::function<std::string(const char*)>, std::string&>> stringFileInfoGetters
            {
                { GetCompanyName, info.companyName },
                { GetFileDescription, info.fileDescription },
                { GetFileVersion, info.fileVersion },
                { GetInternalName, info.internalName },
                { GetLegalCopyright, info.legalCopyright },
                { GetOriginalFilename, info.originalFilename },
                { GetProductName, info.productName },
                { GetProductVersion, info.productVersion }
            };
            char* stringValue = NULL;   // 내부 메모리를 가리키는 포인터, 해제 X

            for (auto getter : stringFileInfoGetters)
            {
                auto GetStringFileInfo = getter.first;
                auto& stringFileInfo = getter.second;

                if (auto queryString = GetStringFileInfo(buffer.get());
                    queryString.length())
                {
                    if (VerQueryValueA(buffer.get(), queryString.c_str(), (LPVOID*)&stringValue, (PUINT)&bufsize))
                    {
                        stringFileInfo = stringValue;
                    }
                }
            }
        }
    }
    return info;
}

void Log(const FileVersionInformation& fileVersion)
{
    logger  << "CompanyName:\t"         << fileVersion.companyName      << std::endl
            << "FileDescription:\t"     << fileVersion.fileDescription  << std::endl
            << "FileVersion:\t"         << fileVersion.fileVersion      << std::endl
            << "InternalName:\t"        << fileVersion.internalName     << std::endl
            << "LegalCopyright:\t"      << fileVersion.legalCopyright   << std::endl
            << "OriginalFilename:\t"    << fileVersion.originalFilename << std::endl
            << "ProductName:\t"         << fileVersion.productName      << std::endl
            << "ProductVersion:\t"      << fileVersion.productVersion   << std::endl;
}
