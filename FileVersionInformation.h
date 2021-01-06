#pragma once
#include <string>
using namespace std::literals;

#include <Windows.h>

namespace LogData
{
    struct FileVersionInformation
    {
        std::string companyName = "(EMPTY)"s;
        std::string fileDescription = "(EMPTY)"s;
        std::string fileVersion = "(EMPTY)"s;
        std::string internalName = "(EMPTY)"s;
        std::string legalCopyright = "(EMPTY)"s;
        std::string originalFilename = "(EMPTY)"s;
        std::string productName = "(EMPTY)"s;
        std::string productVersion = "(EMPTY)"s;
    };

    FileVersionInformation GetFileVersionInformation(const std::string& fileName);

    void Log(const FileVersionInformation& fileInfo);
}
